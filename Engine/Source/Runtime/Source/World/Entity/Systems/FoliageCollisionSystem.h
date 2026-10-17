#pragma once

#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "FoliageCollisionSystem.generated.h"

namespace Lumina
{
    // Bakes static physics bodies for collision-enabled foliage types; rebakes when instances change.
    REFLECT()
    class RUNTIME_API SFoliageCollisionSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnUpdate(const SSystemContext& Context) override;
        void OnTeardown(const SSystemContext& Context) override;
    };
}
