#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Reflector/Types/PropertyFlags.h"
#include "Reflector/Types/StructReflectItem.h"
#include "Reflector/Utils/MetadataUtils.h"

namespace Lumina::Reflection
{
    class FCodeWriter;
}

namespace Lumina
{
    // Which reflected type a property names, so the generated file declares its accessor before the trait reaches for it.
    enum class EReflectedLink : uint8_t
    {
        None,
        Struct,
        Class,
        Enum,
    };

    class FReflectedProperty : public IStructReflectable
    {
    public:

        virtual ~FReflectedProperty() = default;

        virtual const char* GetTypeName() = 0;

        virtual EReflectedLink GetLink() const { return EReflectedLink::None; }

        // Whether TypeName must be in the database, which an intrinsic class such as CObject never is.
        virtual bool RequiresReflectedType() const { return GetLink() != EReflectedLink::None; }

        void GenerateMetadata(const std::string& InMetadata) override;

        // Names the two contradicting specifiers and which to drop. Runs after GenerateMetadata, where the cursor is still at hand.
        bool FindConflictingSpecifiers(std::string& OutMessage) const;

        // One FPropertyDesc row, whose type the runtime's trait derives from the member itself.
        void EmitDescriptor(Reflection::FCodeWriter& Writer) const;

        std::vector<FMetadataPair>    Metadata;
        EPropertyFlags                PropertyFlags;
        std::string                   RawTypeName;
        std::string                   TypeName;
        std::string                   Namespace;
        std::string                   Name;

        // The type offsetof and decltype are taken against, the owner or a function's frame.
        std::string                   Outer;
        std::string                   GetterFunc;
        std::string                   SetterFunc;

        // The type a ReflectAs specifier presents the member as, in place of the one it is declared with.
        std::string                   ReflectedAsType;

        // An element, key, value, payload or argument, described by the trait of what holds it.
        bool                          bInner = false;
    };
}
