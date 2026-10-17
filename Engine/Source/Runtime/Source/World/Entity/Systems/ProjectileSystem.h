#pragma once
#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "ProjectileSystem.generated.h"

namespace Lumina
{
    // Sweeps every SProjectileComponent forward each frame, reports the first hit, and despawns on hit or
    // lifetime expiry. Runs in PrePhysics so movement lands before the physics step and rendering.
    REFLECT()
    class RUNTIME_API SProjectileSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnUpdate(const SSystemContext& Context) override;
    };
}
