#include "ReflectedType.h"

#include "Reflector/CodeGeneration/CodeWriter.h"
#include "Reflector/CodeGeneration/ReflectionNames.h"
#include "Reflector/ReflectionCore/ReflectedHeader.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"

namespace Lumina::Reflection
{
    void FReflectedClass::DefineInitialHeader(FCodeWriter& Writer, const std::string& FileID)
    {
        const std::string Api = Names::ProjectApiMacro(Header->Project->Name);
        const std::string Package = Names::TypePackage(Header->Project->Name);

        if (!Namespace.empty())
        {
            Writer.Linef("namespace %s { class %s; }", Namespace.c_str(), DisplayName.c_str());
        }
        else
        {
            Writer.Linef("\tclass %s;", DisplayName.c_str());
        }

        Writer.Linef("template<> %s Lumina::CClass* Lumina::StaticClass<%s>();", Api.c_str(), EmittedCppQualifiedName().c_str());

        Writer.Linef("#define %s_%u_CLASS \\", FileID.c_str(), LineNumber);
        Writer.Macro("public:");

        // Exports only GetPrivateStaticClass() so the type is referenceable without force-exporting members.
        const char* ClassApi = HasMetadata("MinimalAPI") ? Api.c_str() : "NO_API";
        Writer.Macrof("\tDECLARE_CLASS(%s, %s, %s, \"%s\", %s)",
            Namespace.c_str(), DisplayName.c_str(), Parent.c_str(), Package.c_str(), ClassApi);
        Writer.Macrof("\tDEFINE_CLASS_FACTORY(%s::%s)", Namespace.c_str(), DisplayName.c_str());
        Writer.Macrof("\tDECLARE_SERIALIZER(%s, %s)", Namespace.c_str(), DisplayName.c_str());
        Writer.FinalizeMacro();
        Writer.BlankLines(2);
    }

    void FReflectedClass::DefineSecondaryHeader(FCodeWriter& Writer, const std::string& FileID)
    {
        Writer.Linef("#define %s_%u_GENERATED_BODY \\", FileID.c_str(), GeneratedBodyLineNumber);
        Writer.Macrof("\t%s_%u_CLASS", FileID.c_str(), LineNumber);
        Writer.Macro("private:");
        Writer.FinalizeMacro();
        Writer.BlankLines(2);
    }

    void FReflectedClass::DeclareImplementation(FCodeWriter& Writer)
    {
        Writer.Linef("// Begin %s", DisplayName.c_str());
        Writer.Linef("IMPLEMENT_CLASS(%s, %s)", Namespace.c_str(), DisplayName.c_str());
        Writer.Line();

        OpenNamespace(Writer);
        EmitFunctionFrames(Writer);

        Writer.Linef("struct %s", TypeInfoName().c_str());
        Writer.BeginBlock();

        EmitMetadataArrays(Writer);
        EmitPropertyTable(Writer);
        EmitFunctionTable(Writer);
        EmitLayoutAsserts(Writer);

        Writer.Linef("static constexpr Lumina::FClassDesc Desc = { .StaticClass = &%s::StaticClass%s%s };",
            EmittedCppQualifiedName().c_str(), MemberTableFields().c_str(),
            MetadataField(Names::FriendlyFromQualified(QualifiedName), Metadata, ESpecifierTarget::Reflect).c_str());

        Writer.PopIndent();
        Writer.Line("};");
        DefinePropertyTable(Writer);
        DefineFunctionParameterTables(Writer);
        DefineFunctionTable(Writer);
        CloseNamespace(Writer);

        EmitTypeFunctions(Writer);

        Writer.Linef("//~ End %s", DisplayName.c_str());
        Writer.Line();
    }
}
