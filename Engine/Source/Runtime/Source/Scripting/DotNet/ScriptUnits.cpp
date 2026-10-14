#include "RuntimePCH.h"
#include "ScriptUnits.h"
#include "DotNetHost.h"
#include "Platform/Filesystem/PlatformFilesystem.h"
#include <string>
#include <cstring>
#include "Containers/Span.h"
#include "Containers/Vector.h"
#include "Containers/String.h"
#include "Core/Engine/Engine.h"
#include "Core/Plugin/Plugin.h"
#include "Core/Plugin/PluginManager.h"
#include "FileSystem/FileSystem.h"
#include "Log/Log.h"
#include "Paths/Paths.h"
#include "Platform/Process/PlatformProcess.h"
#include "nlohmann/json.hpp"

namespace Lumina::DotNet
{
    FString Join(FStringView Left, FStringView Right)
    {
        FString Result(Left.data(), Left.size());
        if (!Result.empty() && Result.back() != '/' && Result.back() != '\\')
        {
            Result.push_back('/');
        }
        Result.append(Right.data(), Right.size());
        return Result;
    }

    FString ParentOf(FStringView Path)
    {
        const size_t Slash = Path.find_last_of("/\\");
        return Slash == FStringView::npos ? FString() : FString(Path.data(), Slash);
    }

    // File name without directory or extension, which for a managed assembly is also its simple name.
    FString StemOf(FStringView Path)
    {
        const size_t Slash = Path.find_last_of("/\\");
        const size_t Start = Slash == FStringView::npos ? 0 : Slash + 1;
        const size_t Dot   = Path.rfind('.');
        const size_t End   = (Dot == FStringView::npos || Dot < Start) ? Path.size() : Dot;
        return FString(Path.data() + Start, End - Start);
    }

    FString NativePath(FStringView P)
    {
        FString Result(P.data(), P.size());
    #if defined(_WIN32)
        Algo::Replace(Result, '/', '\\');
    #endif
        return Result;
    }

    // Scripts are source rather than VFS content, so a code-only plugin still gets its scripts compiled.
    template<typename TVisit>
    void ForEachScriptSource(const FString& DiskDir, TVisit&& Visit)
    {
        if (DiskDir.empty() || !Filesystem::IsDirectory(DiskDir))
        {
            return;
        }

        Filesystem::IterateDirectoryRecursive(DiskDir, [&Visit](const Filesystem::FDirectoryEntry& Entry)
        {
            if (Entry.IsDirectory() || Entry.GetExtension() != FStringView(".cs"))
            {
                return;
            }

            const FStringView P = Entry.FullPath;
            if (P.find("/obj/") != FStringView::npos || P.find("/bin/") != FStringView::npos)
            {
                return;
            }
            Visit(P);
        });
    }

    void GatherSourcesUnder(const FString& DiskDir, TVector<FGatheredSource>& Out)
    {
        ForEachScriptSource(DiskDir, [&Out](FStringView P)
        {
            FGatheredSource Src;
            if (!Filesystem::ReadFile(Src.Text, P))
            {
                return;
            }

            Src.Path.assign(P.data(), P.size());
            Out.push_back(std::move(Src));
        });
    }
    

    // Descriptor-declared references, resolved (restoring packages when the declaration changed).
    void ApplyScriptReferences(FScriptUnit& Unit, FStringView DescriptorPath, FStringView RootDir)
    {
        if (DescriptorPath.empty() || !Filesystem::Exists(DescriptorPath))
        {
            return;
        }
        if (!ParseScriptReferences(DescriptorPath, RootDir, Unit.Declared))
        {
            return;
        }

        const FString RestoreDir = Unit.IntermediateDir.empty()
            ? FString()
            : Unit.IntermediateDir + "/Packages";

        if (!ResolveScriptReferences(Unit.Name, RestoreDir, Unit.Declared, Unit.References))
        {
            LOG_ERROR("C#: unit '{}' will compile without its declared references.", Unit.Name);
        }
    }

    // The engine convention is one descriptor named for the project beside its content.
    FString FindProjectDescriptor(FStringView ProjectDir)
    {
        FString Found;
        Filesystem::IterateDirectory(ProjectDir, [&Found](const Filesystem::FDirectoryEntry& Entry)
        {
            if (Entry.IsFile() && Entry.GetExtension() == ".lproject")
            {
                Found.assign(Entry.FullPath.data(), Entry.FullPath.size());
                return Filesystem::EVisit::Stop;
            }
            return Filesystem::EVisit::Continue;
        });
        return Found;
    }

    // Each unit compiles into its OWN DLL, and a disabled plugin produces no unit at all.
    TVector<FScriptUnit> BuildScriptUnits()
    {
        TVector<FScriptUnit> Units;
        TVector<FString>     PluginNames;

        for (const FPlugin* Plugin : FPluginManager::Get().GetAllPlugins())
        {
            if (Plugin == nullptr || !Plugin->IsEnabled())
            {
                continue;
            }

            FScriptUnit Unit;
            Unit.Name.assign(Plugin->GetName().data(), Plugin->GetName().size());
            const FString PluginDir(Plugin->GetDirectory().data(), Plugin->GetDirectory().size());
            Unit.DiskDir         = PluginDir + "/Scripts";
            Unit.BinaryDir       = PluginDir + "/Binaries/DotNet";
            Unit.IntermediateDir = PluginDir + "/Intermediates/DotNet/" + Unit.Name;
            Unit.AssemblyPath    = Unit.BinaryDir + "/" + Unit.Name + ".dll";
            for (const FPluginDependency& Dep : Plugin->GetDescriptor().Dependencies)
            {
                Unit.Deps.push_back(Dep.Name);
            }

            ApplyScriptReferences(Unit, Plugin->GetDescriptorPath(), PluginDir);

            PluginNames.push_back(Unit.Name);
            Units.push_back(std::move(Unit));
        }

        {
            FScriptUnit Game;
            Game.Name = "Game";
            if (GEngine != nullptr)
            {
                const FFixedString ScriptsDir = GEngine->GetProjectScriptsDirectory();
                Game.DiskDir.assign(ScriptsDir.data(), ScriptsDir.size());

                const FStringView ProjectPath = GEngine->GetProjectPath();
                if (!ProjectPath.empty())
                {
                    const FString Root(ProjectPath.data(), ProjectPath.size());
                    Game.BinaryDir       = Root + "/Binaries/DotNet";
                    Game.IntermediateDir = Root + "/Intermediates/DotNet/Game";
                    Game.AssemblyPath    = Game.BinaryDir + "/Game.dll";

                    ApplyScriptReferences(Game, FindProjectDescriptor(Root), Root);
                }
            }
            Game.Deps = PluginNames;
            Units.push_back(std::move(Game));
        }

        {
            FScriptUnit Engine;
            Engine.Name = "Engine";
            Engine.DiskDir = Paths::GetEngineResourceDirectory() + "/Scripts";
            // Engine example scripts live next to the engine binaries (a sibling of DotNet/Managed, not in it).
            const FString ExeDir = ParentOf(Platform::GetCurrentProcessPath());
            Engine.BinaryDir       = NativePath(Join(ExeDir, "DotNet"));
            Engine.IntermediateDir = NativePath(Join(ExeDir, "DotNet/obj/Engine"));
            Engine.AssemblyPath    = Engine.BinaryDir + "/Engine.dll";
            Units.push_back(std::move(Engine));
        }

        return Units;
    }

    // Returns empty when no manifest is present, as in a dev run or a project with no C#.
    TVector<FScriptUnit> BuildCookedScriptUnits()
    {
        TVector<FScriptUnit> Units;

        // A package keeps its script assemblies with LuminaSharp in the data folder's Managed directory.
        const FString ScriptsDir   = Join(Paths::GetGameDataDirectory(), "Managed");
        const FString ManifestPath = Join(ScriptsDir, "scripts.manifest.json");

        FString Text;
        if (!Filesystem::ReadFile(Text, ManifestPath))
        {
            return Units;
        }

        nlohmann::json J;
        try
        {
            J = nlohmann::json::parse(Text.c_str());
        }
        catch (const std::exception& Err)
        {
            LOG_ERROR("C#: failed to parse cooked script manifest '{}': {}", ManifestPath, Err.what());
            return Units;
        }

        const nlohmann::json& Arr = J.contains("Units") ? J["Units"] : J;
        if (!Arr.is_array())
        {
            return Units;
        }

        for (const nlohmann::json& Entry : Arr)
        {
            if (!Entry.is_object())
            {
                continue;
            }
            const std::string Name = Entry.value("Name", std::string());
            if (Name.empty())
            {
                continue;
            }

            FScriptUnit Unit;
            Unit.Name = FString(Name.c_str());
            const std::string Dll = Entry.value("Dll", Name + ".dll");
            Unit.AssemblyPath = Join(ScriptsDir, FStringView(Dll.c_str(), Dll.size()));

            if (Entry.contains("Deps") && Entry["Deps"].is_array())
            {
                for (const nlohmann::json& Dep : Entry["Deps"])
                {
                    if (Dep.is_string())
                    {
                        Unit.Deps.push_back(FString(Dep.get<std::string>().c_str()));
                    }
                }
            }

            // Staged beside the unit DLLs, so the cooked game never needs the NuGet cache.
            if (Entry.contains("References") && Entry["References"].is_array())
            {
                for (const nlohmann::json& Reference : Entry["References"])
                {
                    if (Reference.is_string())
                    {
                        const std::string ReferenceName = Reference.get<std::string>();
                        Unit.References.push_back(Join(ScriptsDir, FStringView(ReferenceName.c_str(), ReferenceName.size())));
                    }
                }
            }
            Units.push_back(std::move(Unit));
        }
        return Units;
    }

    // So frequent reloads do not churn the files and the IDE does not reload projects each time.
    void WriteTextIfChanged(FStringView Path, const std::string& Content)
    {
        FString Existing;
        if (Filesystem::ReadFile(Existing, Path)
            && Existing.size() == Content.size()
            && ::memcmp(Existing.data(), Content.data(), Content.size()) == 0)
        {
            return;
        }

        const TSpan<const uint8> Bytes(reinterpret_cast<const uint8*>(Content.data()), Content.size());
        if (Filesystem::WriteFile(Path, Bytes))
        {
            LOG_DISPLAY("Generated C# script project: {}", Path);
        }
    }

    FString CompiledMarkerPath(const FString& DllPath)
    {
        return DllPath + ".compiled";
    }

    // Stable across regenerations, so the IDE does not treat each regen as a brand-new project.
    std::string MakeStableGuid(const FString& Seed)
    {
        auto Fnv = [](const char* S, uint64 Basis) -> uint64
        {
            uint64 H = Basis;
            for (; *S != '\0'; ++S)
            {
                H = (H ^ static_cast<uint64>(static_cast<uint8>(*S))) * 1099511628211ULL;
            }
            return H;
        };

        const uint64 A = Fnv(Seed.c_str(), 14695981039346656037ULL);
        const uint64 B = Fnv(Seed.c_str(), 14695981039346656037ULL ^ 0x9E3779B97F4A7C15ULL);
        uint8 Bytes[16];
        for (int Index = 0; Index < 8; ++Index)
        {
            Bytes[Index]     = static_cast<uint8>(A >> (Index * 8));
            Bytes[8 + Index] = static_cast<uint8>(B >> (Index * 8));
        }

        char Buf[40];
        std::snprintf(Buf, sizeof(Buf),
            "{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            Bytes[0], Bytes[1], Bytes[2], Bytes[3], Bytes[4], Bytes[5], Bytes[6], Bytes[7],
            Bytes[8], Bytes[9], Bytes[10], Bytes[11], Bytes[12], Bytes[13], Bytes[14], Bytes[15]);
        return std::string(Buf);
    }

    // The absolute path of the generated IDE project for a unit ("<DiskDir>/<Name>.Scripts.csproj").
    FString UnitProjectPath(const FScriptUnit& Unit)
    {
        return NativePath(Join(Unit.DiskDir, Unit.Name + ".Scripts.csproj"));
    }

    // Matches the runtime cross-assembly references exactly, so IntelliSense sees the same graph.
    std::string BuildCsprojXml(const FScriptUnit& Unit, const TVector<FScriptUnit>& AllUnits, const FString& LuminaSharpDll)
    {
        std::string Xml;
        Xml += "<Project Sdk=\"Microsoft.NET.Sdk\">\n";
        Xml += "  <!-- GENERATED for IDE IntelliSense only (run \"dotnet.genprojects\" to refresh).\n";
        Xml += "       Scripts are compiled at runtime by the engine; this project is never the runtime\n";
        Xml += "       path and is overwritten on reload. -->\n";
        Xml += "  <PropertyGroup>\n";
        Xml += "    <TargetFramework>net10.0</TargetFramework>\n";
        Xml += "    <Nullable>enable</Nullable>\n";
        Xml += "    <ImplicitUsings>disable</ImplicitUsings>\n";
        Xml += "    <EnableDefaultItems>true</EnableDefaultItems>\n";
        // Generated reflected-type bindings read properties through raw pointers.
        Xml += "    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>\n";
        Xml += "    <AssemblyName>" + std::string(Unit.Name.c_str()) + "</AssemblyName>\n";
        // An IDE build produces the same artifact and nothing lands in the engine library's output.
        if (!Unit.BinaryDir.empty())
        {
            Xml += "    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>\n";
            Xml += "    <OutputPath>" + std::string(Unit.BinaryDir.c_str()) + "/</OutputPath>\n";
        }
        if (!Unit.IntermediateDir.empty())
        {
            Xml += "    <IntermediateOutputPath>" + std::string(Unit.IntermediateDir.c_str()) + "/</IntermediateOutputPath>\n";
        }
        Xml += "  </PropertyGroup>\n";
        Xml += "  <ItemGroup>\n";
        Xml += "    <Reference Include=\"LuminaSharp\">\n";
        Xml += "      <HintPath>" + std::string(LuminaSharpDll.c_str()) + "</HintPath>\n";
        Xml += "    </Reference>\n";
        // The generator the runtime compile loads; without it partial NativeCall bodies are missing.
        Xml += "    <Analyzer Include=\""
            + std::string(NativePath(Join(ParentOf(LuminaSharpDll), "LuminaSharp.Generators.dll")).c_str())
            + "\" />\n";
        // Matches the import ScriptCompiler adds to every runtime compile, so LoadObject<T> resolves the same in the IDE.
        Xml += "    <Using Include=\"LuminaSharp.ObjectCore\" Static=\"true\" />\n";
        Xml += "  </ItemGroup>\n";

        // Named as packages rather than as their restored DLLs, so the IDE restores them the usual way.
        if (!Unit.Declared.Packages.empty() || !Unit.Declared.Assemblies.empty())
        {
            Xml += "  <ItemGroup>\n";
            for (const FScriptPackageRef& Package : Unit.Declared.Packages)
            {
                Xml += "    <PackageReference Include=\"" + std::string(Package.Name.c_str())
                    + "\" Version=\"" + std::string(Package.Version.c_str()) + "\" />\n";
            }
            for (const FString& Assembly : Unit.Declared.Assemblies)
            {
                const FString Stem = StemOf(Assembly);
                Xml += "    <Reference Include=\"" + std::string(Stem.c_str()) + "\">\n";
                Xml += "      <HintPath>" + std::string(NativePath(Assembly).c_str()) + "</HintPath>\n";
                Xml += "    </Reference>\n";
            }
            Xml += "  </ItemGroup>\n";
        }

        std::string Refs;
        for (const FString& DepName : Unit.Deps)
        {
            const FScriptUnit* Dep = nullptr;
            for (const FScriptUnit& Candidate : AllUnits)
            {
                if (Candidate.Name == DepName)
                {
                    Dep = &Candidate;
                    break;
                }
            }
            if (Dep == nullptr || Dep->DiskDir.empty() || !Filesystem::Exists(Dep->DiskDir))
            {
                continue; // the dependency ships no scripts on disk; nothing to reference
            }
            Refs += "    <ProjectReference Include=\"" + std::string(UnitProjectPath(*Dep).c_str()) + "\" />\n";
        }
        if (!Refs.empty())
        {
            Xml += "  <ItemGroup>\n" + Refs + "  </ItemGroup>\n";
        }

        Xml += "</Project>\n";
        return Xml;
    }

    // Placed at the project root, and skipped when no project is loaded.
    void WriteScriptSolution(const TVector<FScriptUnit>& Units)
    {
        if (GEngine == nullptr || GEngine->GetProjectPath().empty())
        {
            return;
        }

        struct FSlnEntry { std::string Name; std::string Path; std::string Guid; };
        TVector<FSlnEntry> Entries;
        for (const FScriptUnit& Unit : Units)
        {
            if (Unit.DiskDir.empty())
            {
                continue;
            }
            if (!Filesystem::Exists(Unit.DiskDir))
            {
                continue;
            }
            Entries.push_back({ std::string(Unit.Name.c_str()) + ".Scripts", std::string(UnitProjectPath(Unit).c_str()), MakeStableGuid(Unit.Name) });
        }
        if (Entries.empty())
        {
            return;
        }

        const char* CsProjTypeGuid = "{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}";
        std::string Sln;
        Sln += "\xEF\xBB\xBF";
        Sln += "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        Sln += "# Visual Studio Version 17\n";
        for (const FSlnEntry& Entry : Entries)
        {
            Sln += "Project(\"" + std::string(CsProjTypeGuid) + "\") = \"" + Entry.Name + "\", \"" + Entry.Path + "\", \"" + Entry.Guid + "\"\n";
            Sln += "EndProject\n";
        }
        Sln += "Global\n";
        Sln += "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
        Sln += "\t\tDebug|Any CPU = Debug|Any CPU\n";
        Sln += "\t\tRelease|Any CPU = Release|Any CPU\n";
        Sln += "\tEndGlobalSection\n";
        Sln += "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
        for (const FSlnEntry& Entry : Entries)
        {
            Sln += "\t\t" + Entry.Guid + ".Debug|Any CPU.ActiveCfg = Debug|Any CPU\n";
            Sln += "\t\t" + Entry.Guid + ".Release|Any CPU.ActiveCfg = Release|Any CPU\n";
        }
        Sln += "\tEndGlobalSection\n";
        Sln += "EndGlobal\n";

        FString ProjectName(GEngine->GetProjectName().data(), GEngine->GetProjectName().size());
        if (ProjectName.empty())
        {
            ProjectName = "Game";
        }
        const FString SlnPath = NativePath(Join(GEngine->GetProjectPath(), ProjectName + ".GameScripts.sln"));
        WriteTextIfChanged(SlnPath, Sln);
    }

    FString FindScriptSourceFile(FStringView TypeName)
    {
        if (TypeName.empty())
        {
            return FString();
        }

        // The stem, since a script is declared in a file named after it by every convention the templates set.
        const size_t Dot = TypeName.find_last_of('.');
        const FStringView Short = Dot == FStringView::npos ? TypeName : TypeName.substr(Dot + 1);

        FString Wanted(Short.data(), Short.size());
        Wanted += ".cs";

        FString Found;
        for (const FScriptUnit& Unit : BuildScriptUnits())
        {
            if (Unit.DiskDir.empty() || !Filesystem::Exists(Unit.DiskDir))
            {
                continue;
            }

            Filesystem::IterateDirectoryRecursive(Unit.DiskDir, [&](const Filesystem::FDirectoryEntry& Entry)
            {
                if (Entry.IsDirectory() || !Found.empty())
                {
                    return true;
                }
                if (Entry.Name.size() == Wanted.size()
                    && EqualsIgnoreCase(Entry.Name, FStringView(Wanted)))
                {
                    Found.assign(Entry.FullPath.data(), Entry.FullPath.size());
                    return false;
                }
                return true;
            });

            if (!Found.empty())
            {
                break;
            }
        }

        return Found;
    }

    void GatherScriptSourceFolders(TVector<FScriptSourceFolder>& Out)
    {
        Out.clear();

        TVector<FScriptUnit> Units;
        for (FScriptUnit& Unit : BuildScriptUnits())
        {
            if (!Unit.DiskDir.empty() && Filesystem::Exists(Unit.DiskDir))
            {
                Units.push_back(Move(Unit));
            }
        }

        auto ToForwardSlashes = [](FString Path)
        {
            for (char& Character : Path)
            {
                Character = Character == '\\' ? '/' : Character;
            }
            return Path;
        };

        // Collected per unit first, since the unit name only earns a folder level when more than one unit has scripts.
        TVector<TVector<FScriptSourceFolder>> PerUnit(Units.size());
        uint32 UnitsWithScripts = 0;
        for (size_t UnitIndex = 0; UnitIndex < Units.size(); ++UnitIndex)
        {
            const FString Root = ToForwardSlashes(Units[UnitIndex].DiskDir);
            Filesystem::IterateDirectoryRecursive(Units[UnitIndex].DiskDir, [&](const Filesystem::FDirectoryEntry& Entry)
            {
                const FStringView Name(Entry.Name);
                if (Entry.IsDirectory() || Name.size() <= 3 || !EqualsIgnoreCase(Name.substr(Name.size() - 3), FStringView(".cs")))
                {
                    return true;
                }

                FString Relative = ToForwardSlashes(FString(Entry.FullPath.data(), Entry.FullPath.size()));
                if (Relative.size() > Root.size() && Relative.compare(0, Root.size(), Root) == 0)
                {
                    Relative.erase(0, Root.size());
                }
                while (!Relative.empty() && Relative.front() == '/')
                {
                    Relative.erase(0, 1);
                }

                const size_t LastSlash = Relative.find_last_of('/');
                FScriptSourceFolder& Found = PerUnit[UnitIndex].emplace_back();
                Found.Stem   = FString(Name.substr(0, Name.size() - 3));
                Found.Folder = LastSlash == FString::npos ? FString() : Relative.substr(0, LastSlash);
                Found.Path   = FString(Entry.FullPath.data(), Entry.FullPath.size());
                return true;
            });
            UnitsWithScripts += PerUnit[UnitIndex].empty() ? 0u : 1u;
        }

        for (size_t UnitIndex = 0; UnitIndex < Units.size(); ++UnitIndex)
        {
            for (FScriptSourceFolder& Found : PerUnit[UnitIndex])
            {
                if (UnitsWithScripts > 1)
                {
                    Found.Folder = Found.Folder.empty() ? Units[UnitIndex].Name : Units[UnitIndex].Name + "/" + Found.Folder;
                }
                Out.push_back(Move(Found));
            }
        }
    }

    void GatherScriptSourceFiles(TVector<FString>& OutPaths)
    {
        OutPaths.clear();

        // The same roots BuildScriptUnits compiles, minus the reference resolution that can restore packages.
        TVector<FString> Roots;
        for (const FPlugin* Plugin : FPluginManager::Get().GetAllPlugins())
        {
            if (Plugin != nullptr && Plugin->IsEnabled())
            {
                Roots.push_back(FString(Plugin->GetDirectory().data(), Plugin->GetDirectory().size()) + "/Scripts");
            }
        }
        if (GEngine != nullptr)
        {
            const FFixedString ScriptsDir = GEngine->GetProjectScriptsDirectory();
            Roots.emplace_back(ScriptsDir.data(), ScriptsDir.size());
        }
        Roots.push_back(Paths::GetEngineResourceDirectory() + "/Scripts");

        for (const FString& Root : Roots)
        {
            ForEachScriptSource(Root, [&OutPaths](FStringView P) { OutPaths.emplace_back(P.data(), P.size()); });
        }
    }

    void GenerateScriptProjects()
    {
        if (!IsInitialized())
        {
            return;
        }

        const FString ExePath = NativePath(Platform::GetCurrentProcessPath());
        const FString Dll = NativePath(Join(ParentOf(ExePath), "DotNet/Managed/LuminaSharp.dll"));

        // The same unit graph the runtime compiles, so the IDE view matches what actually builds.
        const TVector<FScriptUnit> Units = BuildScriptUnits();
        for (const FScriptUnit& Unit : Units)
        {
            if (Unit.DiskDir.empty())
            {
                continue;
            }
            std::error_code Ec;
            if (!Filesystem::Exists(Unit.DiskDir))
            {
                continue;
            }
            WriteTextIfChanged(UnitProjectPath(Unit), BuildCsprojXml(Unit, Units, Dll));
        }

        WriteScriptSolution(Units);
    }
}
