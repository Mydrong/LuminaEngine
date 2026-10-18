#include "RuntimePCH.h"
#include "TweenSystem.h"

#include "World/Subsystems/TweenManager.h"
#include "World/World.h"

namespace Lumina
{
    void STweenSystem::Configure()
    {
        RequireUpdate(EUpdateStage::FrameStart, EUpdatePriority::Highest);

        // While the world is paused only the tweens that ignore time scale move, which keeps pause menus animated.
        RequireUpdate(EUpdateStage::Paused, EUpdatePriority::Highest);
    }

    bool STweenSystem::HasWork(const SSystemContext& Context)
    {
        const FTweenManager* Tweens = GetWorld() != nullptr ? GetWorld()->GetEntityRegistry().Ctx().Find<FTweenManager>() : nullptr;
        return Tweens != nullptr && Tweens->HasTweens();
    }

    void STweenSystem::OnUpdate(const SSystemContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        ECS::FRegistry& Registry = Context.GetRegistry();

        FTweenManager& Tweens = Registry.Ctx().Get<FTweenManager>();
        Tweens.SetWorldRegistry(&Registry);

        const bool  bPaused  = Context.GetUpdateStage() == EUpdateStage::Paused;
        const float Unscaled = GetWorld() != nullptr ? (float)GetWorld()->GetRealDeltaTime() : (float)Context.GetDeltaTime();
        Tweens.Tick(bPaused ? 0.0f : (float)Context.GetDeltaTime(), Unscaled);
    }
}
