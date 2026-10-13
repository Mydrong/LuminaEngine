#include "ReflectedProperty.h"

#include "Reflector/CodeGeneration/CodeWriter.h"
#include "Reflector/ReflectionSpecifiers.h"
#include "Reflector/Types/PropertyFlags.h"
#include "Reflector/Types/ReflectedType.h"
#include "Reflector/Utils/MetadataUtils.h"

namespace Lumina
{
    void FReflectedProperty::EmitDescriptor(Reflection::FCodeWriter& Writer) const
    {
        const std::string Member = Outer + "::" + Name;
        const std::string Value = ReflectedAsType.empty() ? ("decltype(" + Member + ")") : ReflectedAsType;

        std::string Row = "{ .Name = \"" + Name + "\", .Offset = offsetof(" + Outer + ", " + Name + "), .Flags = "
            + PropertyFlagsToString(PropertyFlags) + ", .Type = Lumina::PropertyTypeOf<" + Value + ">";

        if (!GetterFunc.empty())
        {
            Row += ", .Getter = +[](const void* Container, void* Out) { *static_cast<" + Value + "*>(Out) = static_cast<" + Value
                + ">(static_cast<const " + Outer + "*>(Container)->" + GetterFunc + "()); }";
        }
        if (!SetterFunc.empty())
        {
            Row += ", .Setter = +[](void* Container, const void* In) { static_cast<" + Outer + "*>(Container)->" + SetterFunc
                + "(*static_cast<const " + Value + "*>(In)); }";
        }
        Row += Reflection::MetadataField(Name, Metadata, Reflection::ESpecifierTarget::Property);

        std::string EditorFlags;
        for (const FMetadataPair& Pair : Metadata)
        {
            if (Reflection::GetSpecifierStorage(Reflection::ESpecifierTarget::Property, Pair.Key) == Reflection::ESpecifierStorage::EditorFlag)
            {
                EditorFlags += (EditorFlags.empty() ? "" : " | ") + std::string("Lumina::EPropertyEditorFlags::") + Pair.Key;
            }
        }
        if (!EditorFlags.empty())
        {
            Row += ", .EditorFlags = " + EditorFlags;
        }

        Writer.Line((Row + " },").c_str());
    }

    void FReflectedProperty::GenerateMetadata(const std::string& InMetadata)
    {
        if (InMetadata.empty())
        {
            return;
        }

        FMetadataParser Parser(InMetadata);
        Metadata = std::move(Parser.Metadata);

        for (const FMetadataPair& MetadataPair : Metadata)
        {
            PropertyFlags |= Reflection::PropertyFlagForSpecifier(MetadataPair.Key);

            if (MetadataPair.Key == "Getter")
            {
                GetterFunc = MetadataPair.Value.empty() ? ("Get" + Name) : MetadataPair.Value;
            }
            else if (MetadataPair.Key == "Setter")
            {
                SetterFunc = MetadataPair.Value.empty() ? ("Set" + Name) : MetadataPair.Value;
            }
        }
    }

    bool FReflectedProperty::FindConflictingSpecifiers(std::string& OutMessage) const
    {
        struct FConflict
        {
            EPropertyFlags A;
            EPropertyFlags B;
            const char*    NameA;
            const char*    NameB;
            const char*    Resolution;
        };

        // Each says what to write instead, since the useful part is which specifier to delete.
        static const FConflict Conflicts[] =
        {
            { EPropertyFlags::Editable, EPropertyFlags::ReadOnly, "Editable", "ReadOnly",
              "ReadOnly already shows the property in the details panel with editing disabled; drop Editable." },

            { EPropertyFlags::ScriptReadOnly, EPropertyFlags::ScriptWritable, "ScriptReadOnly", "ScriptWritable",
              "A script property is either readable-only or writable; keep one." },

            { EPropertyFlags::ScriptHidden, EPropertyFlags::ScriptReadOnly, "ScriptHidden", "ScriptReadOnly",
              "ScriptHidden removes the property from script entirely, so an access specifier cannot apply." },

            { EPropertyFlags::ScriptHidden, EPropertyFlags::ScriptWritable, "ScriptHidden", "ScriptWritable",
              "ScriptHidden removes the property from script entirely, so an access specifier cannot apply." },
        };

        for (const FConflict& Conflict : Conflicts)
        {
            if (EnumHasAllFlags(PropertyFlags, Conflict.A | Conflict.B))
            {
                OutMessage = std::string("PROPERTY '") + Name + "' declares both '" + Conflict.NameA
                           + "' and '" + Conflict.NameB + "', which conflict. " + Conflict.Resolution;
                return true;
            }
        }

        return false;
    }
}
