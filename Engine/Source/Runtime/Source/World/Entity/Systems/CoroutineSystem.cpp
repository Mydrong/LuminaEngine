#include "RuntimePCH.h"
#include "CoroutineSystem.h"
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
        World->GetCoroutineManager().Tick(World->GetWorldDeltaTime(), World->GetRealDeltaTime(), bPaused);
    }
}
