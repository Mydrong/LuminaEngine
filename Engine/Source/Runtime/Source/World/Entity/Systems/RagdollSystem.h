#pragma once
#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "RagdollSystem.generated.h"

namespace Lumina
{
    // Bridges SRagdollComponent to the physics scene: on PrePhysics it creates and destroys the ragdoll
    // (seeded from the current animation pose, so it must run after SAnimationSystem); on PostPhysics it
    // reads the simulated bodies back into the skeletal mesh's bone transforms.
    REFLECT()
    class SRagdollSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

    public:

        void OnUpdate(const SSystemContext& Context) override;
    };
}
