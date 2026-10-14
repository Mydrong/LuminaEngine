#pragma once

#include "Core/Object/ObjectMacros.h"
#include "PersistentComponent.generated.h"

namespace Lumina
{
    // Carries the entity, its children and its running scripts into every level the game travels to.
    REFLECT(Component, Category = "Gameplay")
    struct RUNTIME_API SPersistentComponent
    {
        GENERATED_BODY()

        // Arriving where a persistent entity with the same tag was placed, it replaces that one and takes its spot.
        PROPERTY(Editable, Category = "Gameplay")
        bool bTakePlacement = true;
    };
}
