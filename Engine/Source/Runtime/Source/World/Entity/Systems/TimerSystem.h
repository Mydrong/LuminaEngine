#pragma once
#include "EntitySystem.h"
#include "TimerSystem.generated.h"

namespace Lumina
{
    // Advances the timer subsystem's manager first at FrameStart, so timer callbacks fire before gameplay systems.
    REFLECT()
    class STimerSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnUpdate(const SSystemContext& Context) override;
    };
}
