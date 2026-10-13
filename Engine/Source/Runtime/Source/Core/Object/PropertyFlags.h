#pragma once

#include "Core/LuminaMacros.h"
#include "Platform/GenericPlatform.h"
#include <iterator>

namespace Lumina
{
    enum class EPropertyFlags : uint32
    {
        None = 0,
#define LE_PROPERTY_FLAG(Name, Bit) Name = BIT(Bit),
#include "EPropertyFlags.inl"
#undef LE_PROPERTY_FLAG
    };

    ENUM_CLASS_FLAGS(EPropertyFlags);

    // Checked name by name against LuminaSharp.EPropertyFlags at bootstrap.
    inline constexpr const char* PropertyFlagNames[] =
    {
#define LE_PROPERTY_FLAG(Name, Bit) #Name,
#include "EPropertyFlags.inl"
#undef LE_PROPERTY_FLAG
    };

    inline constexpr uint32 PropertyFlagValues[] =
    {
#define LE_PROPERTY_FLAG(Name, Bit) (1u << (Bit)),
#include "EPropertyFlags.inl"
#undef LE_PROPERTY_FLAG
    };

    static_assert(std::size(PropertyFlagNames) == std::size(PropertyFlagValues));

    // Hints only the editor reads, each named exactly as the PROPERTY specifier that sets it.
#define LUMINA_PROPERTY_EDITOR_FLAGS(X)     X(Color) X(NoDrag) X(Multiline) X(FilePath) X(RawJson) X(NoReorder) X(NoResize) X(DefaultCollapsed) X(EditConditionHides) X(RequiresRecook)

    namespace PropertyEditorFlagBit
    {
        enum : uint16
        {
#define LUMINA_EDITOR_FLAG_BIT(Name) Name,
            LUMINA_PROPERTY_EDITOR_FLAGS(LUMINA_EDITOR_FLAG_BIT)
#undef LUMINA_EDITOR_FLAG_BIT
            Count
        };
    }

    enum class EPropertyEditorFlags : uint16
    {
        None = 0,
#define LUMINA_EDITOR_FLAG_VALUE(Name) Name = 1u << PropertyEditorFlagBit::Name,
        LUMINA_PROPERTY_EDITOR_FLAGS(LUMINA_EDITOR_FLAG_VALUE)
#undef LUMINA_EDITOR_FLAG_VALUE
    };

    ENUM_CLASS_FLAGS(EPropertyEditorFlags);

    // Indexed by bit, so a name's position is the flag it sets.
    inline constexpr const char* PropertyEditorFlagNames[] =
    {
#define LUMINA_EDITOR_FLAG_NAME(Name) #Name,
        LUMINA_PROPERTY_EDITOR_FLAGS(LUMINA_EDITOR_FLAG_NAME)
#undef LUMINA_EDITOR_FLAG_NAME
    };

    static_assert(PropertyEditorFlagBit::Count <= 16);

    // Mirrored by LuminaSharp.EPropertyType and the Reflector's own copy, and the order is the wire format.
    enum class EPropertyTypeFlags : uint8
    {
        None = 0,
#define LE_PROPERTY_TYPE(Name) Name,
#include "EPropertyTypeFlags.inl"
#undef LE_PROPERTY_TYPE
        Count,
    };

    ENUM_CLASS_FLAGS(EPropertyTypeFlags);

    inline constexpr const char* PropertyTypeFlagNames[] =
    {
        "None",
#define LE_PROPERTY_TYPE(Name) #Name "Property",
#include "EPropertyTypeFlags.inl"
#undef LE_PROPERTY_TYPE
    };

    // Checked name by name against LuminaSharp.EPropertyType at bootstrap.
    inline constexpr const char* PropertyTypePlainNames[] =
    {
        "None",
#define LE_PROPERTY_TYPE(Name) #Name,
#include "EPropertyTypeFlags.inl"
#undef LE_PROPERTY_TYPE
    };

    static_assert(std::size(PropertyTypeFlagNames) == (size_t)EPropertyTypeFlags::Count);
    static_assert(std::size(PropertyTypePlainNames) == (size_t)EPropertyTypeFlags::Count);

    inline const char* PropertyTypeToString(EPropertyTypeFlags Flag)
    {
        // A header read from a corrupt package can carry any kind byte.
        const uint16 Index = static_cast<uint16>(Flag);
        return Index < (uint16)EPropertyTypeFlags::Count ? PropertyTypeFlagNames[Index] : "Invalid";
    }
}
