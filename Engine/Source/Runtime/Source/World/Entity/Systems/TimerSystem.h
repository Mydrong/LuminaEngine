#pragma once
#include "EntitySystem.h"
#include "TimerSystem.generated.h"

namespace Lumina
{
    // Advances the per-world FTimerManager (a ctx singleton) once per frame. Runs first at FrameStart so
    // Timer callbacks fire before gameplay systems, and run exclusive because a callback is arbitrary code.
    REFLECT()
    class STimerSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnUpdate(const SSystemContext& Context) override;
    };
}
