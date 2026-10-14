#include "RuntimePCH.h"

#include "TweenLibrary.h"

#include "Memory/SmartPtr.h"
#include "World/Subsystems/TweenManager.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // Shared with whatever step captured it, so the managed handle frees when the tween is destroyed.
        using FTweenCallback = TSharedPtr<FScriptCallbackOwner>;

        FTweenCallback TweenLibraryMakeCallback(SScriptCallback Callback)
        {
            return MakeShared<FScriptCallbackOwner>(Callback);
        }

        // Rebuilt per call, since the builder is just a manager pointer plus a generational id.
        bool ResolveTween(CWorld* World, uint32 Id, FTween& Out)
        {
            if (World == nullptr)
            {
                return false;
            }

            FTweenHandle Handle;
            Handle.Handle = ECS::FEntity::FromPacked(Id);
            Out = FTween(&World->GetTweenManager(), Handle);
            return true;
        }
    }

    uint32 CTweenLibrary::Create(CWorld* World, ECS::FEntity Owner)
    {
        if (World == nullptr)
        {
            return (uint32)ECS::NullEntity;
        }

        FTweenManager& Manager = World->GetTweenManager();
        const FTween Tween = Owner != ECS::NullEntity ? Manager.CreateForEntity(Owner) : Manager.Create();
        return (uint32)Tween.GetHandle().Handle;
    }

    void CTweenLibrary::MoveTo(CWorld* World, uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.MoveTo(Target, Destination, Duration);
        }
    }

    void CTweenLibrary::RotateTo(CWorld* World, uint32 Tween, ECS::FEntity Target, FQuat Destination, float Duration)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.RotateTo(Target, Destination, Duration);
        }
    }

    void CTweenLibrary::ScaleTo(CWorld* World, uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.ScaleTo(Target, Destination, Duration);
        }
    }

    void CTweenLibrary::ValueTo(CWorld* World, uint32 Tween, float From, float To, float Duration, SScriptCallback OnValue)
    {
        FTween Builder;
        if (!ResolveTween(World, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnValue);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnValue);
        Builder.To(From, To, Duration, [Owner](const float& Value)
        {
            Owner->Invoke(Scripting::PackFloatPayload(Value));
        });
    }

    void CTweenLibrary::Interval(CWorld* World, uint32 Tween, float Duration)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.Interval(Duration);
        }
    }

    void CTweenLibrary::Call(CWorld* World, uint32 Tween, SScriptCallback OnCall)
    {
        FTween Builder;
        if (!ResolveTween(World, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnCall);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnCall);
        Builder.Call([Owner] { Owner->Invoke(0); });
    }

    void CTweenLibrary::OnFinished(CWorld* World, uint32 Tween, SScriptCallback OnFinished)
    {
        FTween Builder;
        if (!ResolveTween(World, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnFinished);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnFinished);
        Builder.OnFinished([Owner] { Owner->Invoke(0); });
    }

    void CTweenLibrary::Trans(CWorld* World, uint32 Tween, EEaseTransition Transition)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.Trans(Transition);
        }
    }

    void CTweenLibrary::Ease(CWorld* World, uint32 Tween, EEaseType Ease)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.Ease(Ease);
        }
    }

    void CTweenLibrary::Delay(CWorld* World, uint32 Tween, float Seconds)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.Delay(Seconds);
        }
    }

    void CTweenLibrary::Parallel(CWorld* World, uint32 Tween)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.Parallel();
        }
    }

    void CTweenLibrary::SetLoops(CWorld* World, uint32 Tween, int32 Count)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.SetLoops(Count);
        }
    }

    void CTweenLibrary::SetSpeedScale(CWorld* World, uint32 Tween, float Scale)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.SetSpeedScale(Scale);
        }
    }

    void CTweenLibrary::SetPaused(CWorld* World, uint32 Tween, bool bPaused)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.SetPaused(bPaused);
        }
    }

    void CTweenLibrary::Kill(CWorld* World, uint32 Tween)
    {
        FTween Builder;
        if (ResolveTween(World, Tween, Builder))
        {
            Builder.Kill();
        }
    }

    bool CTweenLibrary::IsRunning(CWorld* World, uint32 Tween)
    {
        FTween Builder;
        return ResolveTween(World, Tween, Builder) && Builder.IsRunning();
    }
}
