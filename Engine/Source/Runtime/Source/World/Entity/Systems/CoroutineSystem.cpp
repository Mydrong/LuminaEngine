#include "RuntimePCH.h"
#include "CoroutineSystem.h"
#include "World/Subsystems/TimerSubsystem.h"
#include "World/World.h"

namespace Lumina
{
    // Exclusive, since a resumed coroutine is arbitrary code.
    void SCoroutineSystem::Configure()
    {
        RequireUpdate(EUpdateStage::FrameStart, EUpdatePriority::Highest);
        RequireUpdate(EUpdateStage::Paused, EUpdatePriority::Highest);
    }

    void SCoroutineSystem::OnUpdate(const SSystemContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        CWorld* World = Context.GetWorld();
        if (World == nullptr)
        {
            return;
        }
        const bool bPaused = Context.GetUpdateStage() == EUpdateStage::Paused;
        World->GetTimers()->GetCoroutineManager().Tick(World->GetWorldDeltaTime(), World->GetRealDeltaTime(), bPaused);
    }
}
