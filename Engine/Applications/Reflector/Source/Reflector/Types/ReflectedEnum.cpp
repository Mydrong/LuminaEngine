#include "ReflectedType.h"

#include "Reflector/CodeGeneration/CodeWriter.h"
#include "Reflector/CodeGeneration/ReflectionNames.h"
#include "Reflector/ReflectionCore/ReflectedHeader.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"

namespace Lumina::Reflection
{
    namespace
    {
        // The forward declaration must name the SAME underlying type as the real one or MSVC rejects it C3433.
        const char* UnderlyingTypeName(uint32_t Size, bool bUnsigned)
        {
            switch (Size)
            {
            case 1:  return bUnsigned ? "uint8"  : "int8";
            case 2:  return bUnsigned ? "uint16" : "int16";
            case 8:  return bUnsigned ? "uint64" : "int64";
            default: return bUnsigned ? "uint32" : "int32";
            }
        }
    }

    void FReflectedEnum::DefineInitialHeader(FCodeWriter& Writer, const std::string& /*FileID*/)
    {
        const std::string Api = Names::ProjectApiMacro(Header->Project->Name);

        // At global scope this would create a second distinct type and any TU seeing both gets C2872.
        const char* Underlying = UnderlyingTypeName(UnderlyingSize, bUnsignedUnderlying);

        if (!Namespace.empty())
        {
            Writer.Linef("namespace %s { enum class %s : %s; }",
                Namespace.c_str(), DisplayName.c_str(), Underlying);
        }
        else
        {
            Writer.Linef("enum class %s : %s;", DisplayName.c_str(), Underlying);
        }

        Writer.Linef("template<> %s Lumina::CEnum* Lumina::StaticEnum<%s>();", Api.c_str(), QualifiedName.c_str());
        Writer.Line();
    }

    void FReflectedEnum::DefineSecondaryHeader(FCodeWriter& /*Writer*/, const std::string& /*FileID*/)
    {
        // Enums have no GENERATED_BODY, since StaticEnum is a specialization declared in the header.
    }

    void FReflectedEnum::DeclareImplementation(FCodeWriter& Writer)
    {
        const std::string MetadataSymbol = Names::FriendlyFromQualified(QualifiedName);

        OpenNamespace(Writer);
        Writer.Linef("struct %s", TypeInfoName().c_str());
        Writer.BeginBlock();

        EmitMetadataArray(Writer, MetadataSymbol, Metadata, ESpecifierTarget::Reflect);

        Writer.Line("static constexpr Lumina::FEnumeratorDesc Enumerators[] =");
        Writer.BeginBlock();
        for (const FConstant& Constant : Constants)
        {
            Writer.Linef("{ \"%s::%s\", %u },", DisplayName.c_str(), Constant.Label.c_str(), Constant.Value);
        }
        Writer.PopIndent();
        Writer.Line("};");
        Writer.Line();

        Writer.Linef("static constexpr Lumina::FEnumDesc Desc = { .Name = \"%s\", .Enumerators = Enumerators, "
            ".NumEnumerators = (Lumina::uint32)std::size(Enumerators)%s };",
            DisplayName.c_str(), MetadataField(MetadataSymbol, Metadata, ESpecifierTarget::Reflect).c_str());

        Writer.PopIndent();
        Writer.Line("};");
        CloseNamespace(Writer);

        EmitTypeFunctions(Writer);
    }
}
