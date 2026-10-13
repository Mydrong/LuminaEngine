#pragma once

#include "Core/Object/ObjectCore.h"
#include "Core/Object/PropertyArena.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class FProperty;

    // Everything the engine needs to know about one property kind, so adding a kind touches one table.
    struct FPropertyKindOps
    {
        FProperty* (*Construct)(const FPropertyOwner& Owner, const FPropertyDesc& Desc) = nullptr;

        // Zero when the footprint depends on a type resolved at registration.
        uint32 Size = 0;
        uint32 Alignment = 0;

        bool bArithmetic = false;

        // Set for a kind that names nothing, which a script layout can therefore build from the kind alone.
        const FPropertyType* BasicType = nullptr;

        NODISCARD bool IsValid() const { return Construct != nullptr; }

        NODISCARD bool GetFixedLayout(uint32& OutSize, uint32& OutAlignment) const
        {
            if (Size == 0)
            {
                return false;
            }
            OutSize = Size;
            OutAlignment = Alignment;
            return true;
        }
    };

    // Never null, since an unregistered kind answers with an entry whose IsValid() is false.
    RUNTIME_API const FPropertyKindOps& GetPropertyKindOps(EPropertyTypeFlags Kind);
}
