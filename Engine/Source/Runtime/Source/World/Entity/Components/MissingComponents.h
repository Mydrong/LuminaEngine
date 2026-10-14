#pragma once

#include "Containers/Name.h"
#include "Containers/Vector.h"
#include "Core/Serialization/PortableBytes.h"
#include "World/ECS/Registry.h"
#include "MissingComponents.generated.h"

namespace Lumina
{
    // A value whose type was not loaded when it was read, kept whole until the type comes back.
    REFLECT()
    struct RUNTIME_API SHeldValue
    {
        GENERATED_BODY()

        PROPERTY(ReadOnly)
        FName TypeName;

        FPortableBytes Data;

        friend FArchive& operator<<(FArchive& Ar, SHeldValue& Held) { return Ar << Held.TypeName << Held.Data; }
    };

    // Components whose type was not loaded, saved and copied with the entity until a load or script reload brings the type back.
    REFLECT(Component, HideInComponentList)
    struct RUNTIME_API SMissingComponents
    {
        GENERATED_BODY()

        PROPERTY(ReadOnly, NoResize, NoReorder, TitleProperty = "TypeName")
        TVector<SHeldValue> Entries;

        bool Serialize(FArchive& Ar)
        {
            Ar << Entries;
            return true;
        }
    };

    namespace MissingComponents
    {
        RUNTIME_API void Hold(ECS::FRegistry& Registry, ECS::FEntity Entity, SHeldValue&& Held);

        // Emplaces every held component whose type now resolves, returning how many came back.
        RUNTIME_API int32 Restore(ECS::FRegistry& Registry, ECS::FEntity Entity);
        RUNTIME_API int32 Restore(ECS::FRegistry& Registry);

        // Moves live components whose C# type a reload removed into the holder, so a save keeps them under their own name.
        RUNTIME_API int32 HoldOrphaned(ECS::FRegistry& Registry);
    }
}
