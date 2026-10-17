#pragma once

#include "World/ECS/Registry.h"

#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "AI/Perception/PerceptionTypes.h"
#include "PerceptionSystem.generated.h"

namespace Lumina
{
    // Runs every perceiver's senses each tick: a spatial-grid sight scan with FOV + line-of-sight (parallel),
    // event-driven hearing/damage, hysteresis + forgetting, and the perceived/lost events. PrePhysics, ahead
    // of PathFollow, so an AI can react and set a move target the same frame; bodies are stable for LoS rays.
    REFLECT()
    class RUNTIME_API SPerceptionSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        // Creates the per-world perception singleton once, serially at world init -- Update runs concurrently
        // with sibling systems, so it must never structurally mutate the shared registry context.
        void OnStartup(const SSystemContext& Context) override;

        void OnUpdate(const SSystemContext& Context) override;
    };
}
