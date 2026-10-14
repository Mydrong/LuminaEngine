#pragma once

#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Scripting/ScriptCallback.h"
#include "World/ECS/Entity.h"
#include "TimerLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    /** World-time timers. The returned id is generational, so a stale one reports inactive after recycling. */
    REFLECT()
    class RUNTIME_API CTimerLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** Runs OnFire after Rate seconds, repeating while bLoop. FirstDelay >= 0 overrides the first wait. */
        FUNCTION()
        static uint32 SetTimer(CWorld* World, float Rate, SScriptCallback OnFire, bool bLoop = false,
            float FirstDelay = -1.0f);

        /** As SetTimer, but cleared automatically when Owner is destroyed. */
        FUNCTION()
        static uint32 SetTimerForEntity(CWorld* World, ECS::FEntity Owner, float Rate, SScriptCallback OnFire,
            bool bLoop = false, float FirstDelay = -1.0f);

        FUNCTION()
        static void Clear(CWorld* World, uint32 Timer);

        FUNCTION()
        static bool IsActive(CWorld* World, uint32 Timer);

        /** Seconds until the next fire. */
        FUNCTION()
        static float GetRemaining(CWorld* World, uint32 Timer);

        /** Seconds elapsed in the current interval. */
        FUNCTION()
        static float GetElapsed(CWorld* World, uint32 Timer);

        FUNCTION()
        static void SetPaused(CWorld* World, uint32 Timer, bool bPaused);

        /** Stops every script-owned timer, so a looper does not tick on into the next generation. */
        FUNCTION()
        static void ClearAllManaged();
    };
}
