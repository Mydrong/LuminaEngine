#include "ReflectedType.h"
#include <algorithm>

#include "Reflector/CodeGeneration/CodeWriter.h"
#include "Reflector/CodeGeneration/ReflectionNames.h"
#include "Reflector/ReflectionCore/ReflectedHeader.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"

namespace Lumina::Reflection
{

    FReflectedStruct::~FReflectedStruct() = default;

    // A template argument list carries commas, which the offsetof macro would read as extra arguments.
    std::string FReflectedStruct::OffsetBaseTypeName() const
    {
        const std::string& Cpp = EmittedCppQualifiedName();
        return Cpp.find('<') == std::string::npos
            ? Cpp
            : ("LRT_Owner_" + Names::FriendlyFromParts(Namespace, DisplayName));
    }

    void FReflectedStruct::PushProperty(std::unique_ptr<FReflectedProperty>&& NewProperty)
    {
        NewProperty->Outer = OffsetBaseTypeName();
        Props.push_back(std::move(NewProperty));
    }

    void FReflectedStruct::PushFunction(std::unique_ptr<FReflectedFunction>&& NewFunction)
    {
        NewFunction->Outer = Namespace.empty() ? DisplayName : (Namespace + "::" + DisplayName);
        Functions.push_back(std::move(NewFunction));
    }

    std::vector<std::unique_ptr<FReflectedProperty>> FReflectedStruct::TakeBackPropertiesFrom(size_t FirstIndex)
    {
        std::vector<std::unique_ptr<FReflectedProperty>> Taken;
        if (FirstIndex >= Props.size())
        {
            return Taken;
        }

        Taken.reserve(Props.size() - FirstIndex);
        for (size_t Index = FirstIndex; Index < Props.size(); ++Index)
        {
            Taken.push_back(std::move(Props[Index]));
        }
        Props.erase(Props.begin() + (std::ptrdiff_t)FirstIndex, Props.end());

        return Taken;
    }

    void FReflectedStruct::EmitMetadataArrays(FCodeWriter& Writer) const
    {
        EmitMetadataArray(Writer, Names::FriendlyFromQualified(QualifiedName), Metadata, ESpecifierTarget::Reflect);

        for (const auto& Prop : Props)
        {
            if (!Prop->bInner)
            {
                EmitMetadataArray(Writer, Prop->Name, Prop->Metadata, ESpecifierTarget::Property);
            }
        }
    }

    void FReflectedStruct::EmitPropertyTable(FCodeWriter& Writer) const
    {
        const bool bAnyMember = std::any_of(Props.begin(), Props.end(), [](const auto& Prop) { return !Prop->bInner; });
        if (!bAnyMember)
        {
            return;
        }

        Writer.Line("static constexpr Lumina::FPropertyDesc Properties[] =");
        Writer.BeginBlock();
        for (const auto& Prop : Props)
        {
            if (!Prop->bInner)
            {
                Prop->EmitDescriptor(Writer);
            }
        }
        Writer.PopIndent();
        Writer.Line("};");
        Writer.Line();
    }

    std::string FReflectedStruct::MemberTableFields() const
    {
        std::string Fields;
        if (std::any_of(Props.begin(), Props.end(), [](const auto& Prop) { return !Prop->bInner; }))
        {
            Fields += ", .Properties = Properties, .NumProperties = (Lumina::uint32)std::size(Properties)";
        }
        if (HasNativeFunctions())
        {
            Fields += ", .Functions = Functions, .NumFunctions = (Lumina::uint32)std::size(Functions)";
        }
        return Fields;
    }

    void FReflectedStruct::DefineInitialHeader(FCodeWriter& Writer, const std::string& /*FileID*/)
    {
        // An alias name belongs to the `using`, which is declared after this header is included, so it gets no forward declaration.
        if (bIsAlias)
        {
            return;
        }

        if (!Namespace.empty())
        {
            Writer.Linef("namespace %s { struct %s; }", Namespace.c_str(), EmittedCppName().c_str());
        }
        else
        {
            Writer.Linef("struct %s;", EmittedCppName().c_str());
        }

        const std::string Api = Names::ProjectApiMacro(Header->Project->Name);
        Writer.Linef("template<> %s Lumina::CStruct* Lumina::StaticStruct<%s>();", Api.c_str(), EmittedCppQualifiedName().c_str());
    }

    void FReflectedStruct::DefineSecondaryHeader(FCodeWriter& Writer, const std::string& FileID)
    {
        // An alias has no struct body to inject GENERATED_BODY into.
        if (bIsAlias)
        {
            Writer.BlankLines(2);
            return;
        }

        Writer.Linef("#define %s_%u_GENERATED_BODY \\", FileID.c_str(), GeneratedBodyLineNumber);
        Writer.Macrof("friend struct %s;", TypeInfoName().c_str());

        // Exports StaticStruct() so the type is referenceable without force-exporting every member.
        if (HasMetadata("MinimalAPI"))
        {
            const std::string Api = Names::ProjectApiMacro(Header->Project->Name);
            Writer.Macrof("static %s class Lumina::CStruct* StaticStruct();", Api.c_str());
        }
        else
        {
            Writer.Macro("static class Lumina::CStruct* StaticStruct();");
        }

        if (!Parent.empty())
        {
            Writer.Macrof("using Super = %s::%s;", Namespace.c_str(), Parent.c_str());
        }

        Writer.FinalizeMacro();
        Writer.BlankLines(2);
    }

    void FReflectedStruct::DeclareImplementation(FCodeWriter& Writer)
    {
        const std::string& Cpp = EmittedCppQualifiedName();

        const std::string OffsetBase = OffsetBaseTypeName();
        if (OffsetBase != Cpp)
        {
            Writer.Linef("using %s = %s;", OffsetBase.c_str(), Cpp.c_str());
            Writer.Line();
        }

        Writer.Linef("// Begin %s", DisplayName.c_str());
        OpenNamespace(Writer);
        EmitFunctionFrames(Writer);

        Writer.Linef("struct %s", TypeInfoName().c_str());
        Writer.BeginBlock();

        Writer.Linef("static Lumina::FStructOps* MakeOps() { return Lumina::MakeStructOps<%s>(); }", Cpp.c_str());
        Writer.Line();

        EmitMetadataArrays(Writer);
        EmitPropertyTable(Writer);
        EmitFunctionTable(Writer);

        const std::string Super = Parent.empty() ? std::string() : (", .Super = &" + Cpp + "::Super::StaticStruct");
        Writer.Linef("static constexpr Lumina::FStructDesc Desc = { .Name = \"%s\"%s, .MakeOps = &MakeOps, "
            ".Size = sizeof(%s), .Alignment = alignof(%s)%s%s };",
            DisplayName.c_str(), Super.c_str(), Cpp.c_str(), Cpp.c_str(), MemberTableFields().c_str(),
            MetadataField(Names::FriendlyFromQualified(QualifiedName), Metadata, ESpecifierTarget::Reflect).c_str());

        Writer.PopIndent();
        Writer.Line("};");
        CloseNamespace(Writer);

        EmitTypeFunctions(Writer, HasMetadata("Component") ? ("Lumina::Meta::RegisterComponentMeta<" + Cpp + ">();") : std::string());

        // An alias has no StaticStruct member to define.
        if (!bIsAlias)
        {
            Writer.Linef("Lumina::CStruct* %s::StaticStruct()", Cpp.c_str());
            Writer.Line("{");
            Writer.Linef("\treturn Lumina::Generated::%s();", GetTypeFunctionName().c_str());
            Writer.Line("}");
        }

        Writer.Linef("//~ End %s", DisplayName.c_str());
        Writer.Line();
    }
}
