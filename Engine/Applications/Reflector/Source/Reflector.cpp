#include "Reflector/Utils/StringOps.h"
#include <fstream>
#include "StringHash.h"
#include "nlohmann/json.hpp"
#include "Reflector/Clang/Utils.h"
#include "Reflector/Diagnostics/HeaderIncludeGraph.h"
#include "Reflector/Diagnostics/LRTDiagnostics.h"
#include "Reflector/ProjectSolution.h"
#include "Reflector/Clang/ClangParser.h"
#include "Reflector/CodeGeneration/CodeGenerator.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"
#include "Reflector/Utils/Timing.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <thread>


using json = nlohmann::json;
using namespace Lumina::Reflection;

namespace
{
    std::string_view TrimSpaces(std::string_view Text)
    {
        while (!Text.empty() && std::isspace((unsigned char)Text.front())) { Text.remove_prefix(1); }
        while (!Text.empty() && std::isspace((unsigned char)Text.back()))  { Text.remove_suffix(1); }
        return Text;
    }

    // The property names an EditCondition reads, in the grammar the editor parses of A, !A, A == B and A != B joined by && or ||.
    std::vector<std::string> EditConditionNames(std::string_view Expression)
    {
        std::vector<std::string> Names;
        while (!Expression.empty())
        {
            const size_t And = Expression.find("&&");
            const size_t Or = Expression.find("||");
            const size_t Split = std::min(And, Or);

            std::string_view Term = TrimSpaces(Expression.substr(0, Split));
            Expression = Split == std::string_view::npos ? std::string_view() : Expression.substr(Split + 2);

            if (!Term.empty() && Term.front() == '!' && (Term.size() < 2 || Term[1] != '='))
            {
                Term.remove_prefix(1);
            }
            const size_t Compare = std::min(Term.find("=="), Term.find("!="));
            Names.emplace_back(TrimSpaces(Term.substr(0, Compare)));
        }
        return Names;
    }

    // Walks the reflected parents, and an unreflected one carries no properties to find.
    bool HasReflectedProperty(const FReflectionDatabase& Db, const FReflectedStruct& Struct, const std::string& Name)
    {
        for (const FReflectedStruct* Current = &Struct; Current != nullptr;)
        {
            for (const auto& Property : Current->Props)
            {
                if (!Property->bInner && Property->Name == Name)
                {
                    return true;
                }
            }

            const std::string& Parent = Current->Parent;
            if (Parent.empty())
            {
                return false;
            }
            const bool bQualified = Parent.find("::") != std::string::npos;
            const std::string Qualified = (bQualified || Current->Namespace.empty()) ? Parent : (Current->Namespace + "::" + Parent);
            const FReflectedType* Found = Db.GetReflectedType<FReflectedType>(Lumina::FStringHash(Qualified));
            if (Found == nullptr || Found->Type == FReflectedType::EType::Enum || Found == Current)
            {
                return false;
            }
            Current = static_cast<const FReflectedStruct*>(Found);
        }
        return false;
    }
}

int main(int argc, char* argv[])
{    
    Lumina::FStringHash::Initialize();

    for (int i = 1; i < argc; ++i)
    {
        GReportTimings |= std::string(argv[i]) == "-timings";
    }

    FScopedPhaseTimer TotalTimer("Total");
    
    std::puts("===============================================");
    std::puts("======== Lumina Reflection Tool (LRT) =========");
    std::puts("===============================================");
    
#if 0
    
    std::string InputFile = "H:/LuminaEngine/Reflection_Files.json";
    
#else
    if (argc < 2)
    {
        FDiagnostics::Get().Errorf({}, EDiagId::DriverMissingInput,
            "Missing command line argument: a Reflection_Files.json path is required.");
        FDiagnostics::Get().PrintSummary();
        return 1;
    }

    std::string InputFile = argv[1];
#endif

    std::ifstream File(InputFile.c_str());
    if (!File.is_open())
    {
        // Anchoring at the JSON path lets a double-click on the build log open the missing file.
        FDiagLocation Loc;
        Loc.File = InputFile;
        FDiagnostics::Get().Errorf(Loc, EDiagId::DriverInputUnreadable,
            "Failed to open Reflector input file '%s'.", InputFile.c_str());
        FDiagnostics::Get().PrintSummary();
        return 1;
    }
    
    
    json Data = json::parse(File);
    
    std::string WorkspaceName     = Data["WorkspaceName"].get<std::string>().c_str();
    std::string WorkspacePath     = Data["WorkspacePath"].get<std::string>().c_str();
    
    FReflectedWorkspace Workspace(WorkspacePath.c_str());

    FScopedPhaseTimer WorkspaceTimer("Workspace registration");

    for (const auto& Project : Data["Projects"])
    {
        std::string ProjectName = Project["Name"].get<std::string>().c_str();
        std::string ProjectPath = Project["Path"].get<std::string>().c_str();
        
        auto ReflectedProject = std::make_unique<FReflectedProject>(&Workspace);
        ReflectedProject->Name = std::move(ProjectName);
        // Normalized so prefix-matching against Header->HeaderPath works without per-call fixups.
        ReflectedProject->Path = Lumina::ClangUtils::NormalizeHeaderPath(std::move(ProjectPath));

        // A reference-only project is parsed so its types are known, but never generated for.
        if (Project.contains("ReferenceOnly"))
        {
            ReflectedProject->bReferenceOnly = Project["ReferenceOnly"].get<bool>();
        }

        // Optional, the build system pins where this project's generated C++ lands.
        if (Project.contains("GeneratedDir") && !Project["GeneratedDir"].get<std::string>().empty())
        {
            std::string GeneratedDir = Project["GeneratedDir"].get<std::string>().c_str();
            ReflectedProject->GeneratedDir = Lumina::ClangUtils::NormalizeHeaderPath(std::move(GeneratedDir));
        }

        // Absent or empty means the module has none, and generated sources then include no PCH.
        if (Project.contains("PrecompiledHeader"))
        {
            ReflectedProject->PrecompiledHeader = Project["PrecompiledHeader"].get<std::string>().c_str();
        }

        // Optional, a plugin or game module routes its C# bindings into its own Scripts/Generated dir.
        if (Project.contains("CSharpBindingsDir") && !Project["CSharpBindingsDir"].get<std::string>().empty())
        {
            std::string CSharpDir = Project["CSharpBindingsDir"].get<std::string>().c_str();
            ReflectedProject->CSharpBindingsDir = Lumina::ClangUtils::NormalizeHeaderPath(std::move(CSharpDir));
        }

        if (Project.contains("RouteTypeBindings"))
        {
            ReflectedProject->bRouteTypeBindings = Project["RouteTypeBindings"].get<bool>();
        }

        for (const auto& IncludeDirJson : Project["IncludeDirs"])
        {
            std::string IncludeDir = IncludeDirJson.get<std::string>().c_str();
            IncludeDir = Lumina::ClangUtils::NormalizeHeaderPath(std::move(IncludeDir));
            ReflectedProject->IncludeDirs.push_back(std::move(IncludeDir));
        }
        
        if (Project.contains("Definitions"))
        {
            for (const auto& DefinitionJson : Project["Definitions"])
            {
                ReflectedProject->Definitions.push_back(DefinitionJson.get<std::string>().c_str());
            }
        }

        if (Project.contains("ForceIncludes"))
        {
            for (const auto& ForceIncludeJson : Project["ForceIncludes"])
            {
                std::string ForceInclude = ForceIncludeJson.get<std::string>().c_str();
                ForceInclude = Lumina::ClangUtils::NormalizeHeaderPath(std::move(ForceInclude));
                ReflectedProject->ForceIncludes.push_back(std::move(ForceInclude));
            }
        }

        for (const auto& ProjectFileJson : Project["Files"])
        {
            std::string ProjectFile = ProjectFileJson.get<std::string>().c_str();
            ProjectFile = Lumina::ClangUtils::NormalizeHeaderPath(std::move(ProjectFile));

            auto ReflectedHeader = std::make_unique<FReflectedHeader>(ReflectedProject.get(), ProjectFile);

            Lumina::FStringHash HeaderHash(ProjectFile);
            ReflectedProject->Headers.emplace(HeaderHash, std::move(ReflectedHeader));
        }
        
        Workspace.AddReflectedProject(std::move(ReflectedProject));
    }

    WorkspaceTimer.Stop();

    // Runs alongside the parse, reading only header paths while the walk writes their include lists.
    FHeaderIncludeGraph Graph;
    std::vector<FHeaderCycle> Cycles;
    double GraphMilliseconds = 0.0;

    // Bailing ahead of the parse bought nothing, since clang rejects a cycle in about ten milliseconds.
    std::thread GraphThread([&Graph, &Cycles, &GraphMilliseconds, &Workspace]
    {
        const auto Start = std::chrono::steady_clock::now();
        Graph.BuildFromWorkspace(&Workspace);
        Cycles = Graph.DetectCycles();
        GraphMilliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - Start).count();
    });

    FClangParser Parser;

    for (int i = 2; i < argc; ++i)
    {
        Parser.bStrictParse |= std::string(argv[i]) == "-strict-parse";
        Parser.bVerboseDiagnostics |= std::string(argv[i]) == "-verbose-diagnostics";
    }

    bool bParseResult;
    {
        FScopedPhaseTimer ParseTimer("Parse (total)");
        bParseResult = Parser.Parse(&Workspace);
    }

    GraphThread.join();
    if (GReportTimings)
    {
        std::printf("[timing] %-30s %8.1f ms (overlapped)\n", "Include graph + cycles", GraphMilliseconds);
    }

    for (const FHeaderCycle& Cycle : Cycles)
    {
        // Build a "A.h -> B.h -> A.h" arrow chain for the message body.
        std::string Arrow;
        for (size_t i = 0; i < Cycle.size(); ++i)
        {
            if (i > 0)
            {
                Arrow += " -> ";
            }
            Arrow += Cycle[i];
        }

        // Anchor at the cycle's first include edge so the build-log error opens the offending line.
        FDiagLocation Loc;
        Loc.File = Cycle.front();
        if (Cycle.size() >= 2)
        {
            Loc.Line = Graph.GetIncludeLine(Cycle[0], Cycle[1]);
        }

        FDiagnostics::Get().Errorf(Loc, EDiagId::CircularHeaderInclude,
            "Circular header include: %s", Arrow.c_str());
    }

    // A cycle makes every later diagnostic noise, so it ends the run before anything is generated.
    if (!Cycles.empty() || !bParseResult)
    {
        FDiagnostics::Get().PrintSummary();
        return 1;
    }

    FScopedPhaseTimer ValidationTimer("Validation");

    // Any header with a reflection macro must end its include block with the matching generated.h.
    for (const auto& Project : Workspace.ReflectedProjects)
    {
        // Someone else's module, already validated by the workspace that owns it.
        if (Project->bReferenceOnly)
        {
            continue;
        }

        for (auto& [_, Header] : Project->Headers)
        {
            if (!Header->bHasReflectionMacros)
            {
                continue;
            }

            // find() not operator[], since the latter inserts empty entries the codegen would emit empty files for.
            auto TypeIt = Parser.ParsingContext.ReflectionDatabase.ReflectedTypes.find(Header.get());
            if (TypeIt != Parser.ParsingContext.ReflectionDatabase.ReflectedTypes.end() && !TypeIt->second.empty())
            {
                // An alias has no GENERATED_BODY to feed, so its header needs no companion include.
                bool bAllAliases = true;
                for (const auto& T : TypeIt->second)
                {
                    if (!T->bIsAlias)
                    {
                        bAllAliases = false;
                        break;
                    }
                }
                if (bAllAliases)
                {
                    continue;
                }
            }

            std::string ExpectedGenerated = Header->FileName + ".generated.h";
            Lumina::StringOps::ToLower(ExpectedGenerated);

            const FIncludeRef* GeneratedInclude = nullptr;
            const FIncludeRef* WrongGenerated   = nullptr;

            constexpr const char* kGeneratedSuffix = ".generated.h";
            constexpr size_t      kGeneratedSuffixLen = 12;

            for (const FIncludeRef& Inc : Header->Includes)
            {
                const bool bEndsWithGen = Inc.Basename.size() >= kGeneratedSuffixLen &&
                    Inc.Basename.compare(Inc.Basename.size() - kGeneratedSuffixLen, kGeneratedSuffixLen, kGeneratedSuffix) == 0;
                if (!bEndsWithGen)
                {
                    continue;
                }

                if (Inc.Basename == ExpectedGenerated)
                {
                    GeneratedInclude = &Inc;
                }
                else if (WrongGenerated == nullptr)
                {
                    WrongGenerated = &Inc;
                }
            }

            FDiagLocation HeaderLoc;
            HeaderLoc.File = Header->HeaderPath;

            if (GeneratedInclude == nullptr)
            {
                if (WrongGenerated != nullptr)
                {
                    FDiagLocation Loc = HeaderLoc;
                    Loc.Line = WrongGenerated->LineNumber;
                    FDiagnostics::Get().Errorf(Loc, EDiagId::WrongGeneratedHeader,
                        "Header includes '%s' but reflection expects '%s'. "
                        "The generated header name must match the source filename stem.",
                        WrongGenerated->Spelling.c_str(), ExpectedGenerated.c_str());
                }
                else
                {
                    HeaderLoc.Line = 1;
                    FDiagnostics::Get().Errorf(HeaderLoc, EDiagId::MissingGeneratedHeader,
                        "Header uses REFLECT/GENERATED_BODY/PROPERTY/FUNCTION but does not "
                        "#include \"%s\". Add it as the last include in the file.",
                        ExpectedGenerated.c_str());
                }
                continue;
            }

            // The generated.h must be the last include, except for an .inl of definitions read after GENERATED_BODY expands.
            const FIncludeRef* LaterInclude = nullptr;
            for (const FIncludeRef& Inc : Header->Includes)
            {
                if (Inc.LineNumber > GeneratedInclude->LineNumber && LaterInclude == nullptr && !Inc.Spelling.ends_with(".inl"))
                {
                    LaterInclude = &Inc;
                }
            }

            if (LaterInclude != nullptr)
            {
                FDiagLocation Loc = HeaderLoc;
                Loc.Line = LaterInclude->LineNumber;
                FDiagnostics::Get().Errorf(Loc, EDiagId::GeneratedHeaderNotLast,
                    "'%s' must be the last #include in this header, but '%s' follows it.",
                    ExpectedGenerated.c_str(), LaterInclude->Spelling.c_str());
            }
        }
    }

    // An unreflected property type leaves its trait nothing to link against, so name the header.
    for (const auto& Project : Workspace.ReflectedProjects)
    {
        if (Project->bReferenceOnly)
        {
            continue;
        }

        for (auto& [_, Header] : Project->Headers)
        {
            auto TypeIt = Parser.ParsingContext.ReflectionDatabase.ReflectedTypes.find(Header.get());
            if (TypeIt == Parser.ParsingContext.ReflectionDatabase.ReflectedTypes.end())
            {
                continue;
            }

            for (const auto& Type : TypeIt->second)
            {
                auto* Struct = dynamic_cast<FReflectedStruct*>(Type.get());
                if (Struct == nullptr)
                {
                    continue;
                }

                for (const auto& Property : Struct->Props)
                {
                    if (const auto Condition = std::find_if(Property->Metadata.begin(), Property->Metadata.end(),
                            [](const FMetadataPair& Pair) { return Pair.Key == "EditCondition"; });
                        Condition != Property->Metadata.end())
                    {
                        for (const std::string& Name : EditConditionNames(Condition->Value))
                        {
                            if (Name.empty() || !HasReflectedProperty(Parser.ParsingContext.ReflectionDatabase, *Struct, Name))
                            {
                                FDiagLocation Loc;
                                Loc.File = Header->HeaderPath;
                                Loc.Line = Type->LineNumber;
                                FDiagnostics::Get().Warningf(Loc, EDiagId::EditConditionUnknown,
                                    "Property '%s' on '%s' has EditCondition \"%s\", which names '%s', not a reflected property "
                                    "of '%s'. The editor ignores the whole condition.",
                                    Property->Name.c_str(), Type->DisplayName.c_str(), Condition->Value.c_str(), Name.c_str(),
                                    Type->DisplayName.c_str());
                            }
                        }
                    }

                    if (!Property->RequiresReflectedType())
                    {
                        continue;
                    }

                    // Empty means deliberately unconstrained (a bare FInstancedStruct), so there is no base to demand.
                    if (Property->TypeName.empty())
                    {
                        continue;
                    }

                    const FReflectedType* Referenced =
                        Parser.ParsingContext.ReflectionDatabase.GetReflectedType<FReflectedType>(
                            Lumina::FStringHash(Property->TypeName));

                    FDiagLocation Loc;
                    Loc.File = Header->HeaderPath;
                    Loc.Line = Type->LineNumber;

                    if (Referenced != nullptr)
                    {
                        const auto* AsStruct = dynamic_cast<const FReflectedStruct*>(Referenced);
                        // A value mirror exists for the C# marshal alone, so a PROPERTY of it would store nothing.
                        if (AsStruct != nullptr && AsStruct->Props.empty()
                            && Referenced->HasMetadata("CSharpValueMirror"))
                        {
                            FDiagnostics::Get().Errorf(Loc, EDiagId::ScriptOnlyPropertyType,
                                "Property '%s' on '%s' has type '%s', which is reflected only as a hand-written C# "
                                "value mirror and carries no members. It can cross to script by value but cannot be "
                                "serialized or edited. Remove the PROPERTY() macro from this field.",
                                Property->Name.c_str(), Type->DisplayName.c_str(), Property->TypeName.c_str());
                        }
                        continue;
                    }

                    FDiagnostics::Get().Errorf(Loc, EDiagId::UnreflectedPropertyType,
                        "Property '%s' on '%s' has type '%s', which is not reflected. "
                        "Give that type a REFLECT() + GENERATED_BODY(), or remove the PROPERTY() macro from this field.",
                        Property->Name.c_str(), Type->DisplayName.c_str(), Property->TypeName.c_str());
                }
            }
        }
    }

    ValidationTimer.Stop();

    if (FDiagnostics::Get().GetErrorCount() != 0)
    {
        FDiagnostics::Get().PrintSummary();
        return 1;
    }

    {
        FScopedPhaseTimer CodegenTimer("Code generation");
        FCodeGenerator CodeGenerator(&Workspace, Parser.ParsingContext.ReflectionDatabase);
        CodeGenerator.GenerateCode();
    }

    Lumina::FStringHash::Shutdown();

    // Non-zero exit halts the build; the diagnostic lines were already printed when emitted.
    FDiagnostics::Get().PrintSummary();
    return FDiagnostics::Get().GetErrorCount() == 0 ? 0 : 1;
}
