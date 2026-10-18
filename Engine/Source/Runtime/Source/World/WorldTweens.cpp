#include "RuntimePCH.h"

#include "Memory/SmartPtr.h"
#include "World/Subsystems/TweenManager.h"
#include "World/World.h"
#include "World/Entity/Components/Component.h"

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

        // Null for a world that has not been initialized, which owns no tween manager yet.
        FTweenManager* FindTweenManager(CWorld& World)
        {
            return World.GetEntityRegistry().Ctx().Find<FTweenManager>();
        }

        // Rebuilt per call, since the builder is just a manager pointer plus a generational id.
        bool ResolveTween(CWorld& World, uint32 Id, FTween& Out)
        {
            FTweenManager* Manager = FindTweenManager(World);
            if (Manager == nullptr)
            {
                return false;
            }

            FTweenHandle Handle;
            Handle.Handle = ECS::FEntity::FromPacked(Id);
            Out = FTween(Manager, Handle);
            return true;
        }

        template <typename T>
        void AddValueByAddress(CWorld& World, uint32 Tween, const T& From, const T& To, float Duration, SScriptCallback OnValue)
        {
            FTween Builder;
            if (!ResolveTween(World, Tween, Builder))
            {
                Scripting::ReleaseScriptCallback(OnValue);
                return;
            }

            FTweenCallback Owner = TweenLibraryMakeCallback(OnValue);
            Builder.To(From, To, Duration, [Owner](const T& Value)
            {
                Owner->Invoke((uint64)(uintptr_t)&Value);
            });
        }

        template <typename T>
        void AddProperty(CWorld& World, uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, const T& To, float Duration)
        {
            FTween Builder;
            if (ResolveTween(World, Tween, Builder))
            {
                Builder.PropertyTo(Target, FindComponentStruct(Component), Path, To, Duration);
            }
        }
    }

    uint32 CWorld::CreateTweenId(ECS::FEntity Owner)
    {
        FTweenManager* Manager = FindTweenManager(*this);
        if (Manager == nullptr)
        {
            return (uint32)ECS::NullEntity;
        }

        const FTween Tween = Owner != ECS::NullEntity ? Manager->CreateForEntity(Owner) : Manager->Create();
        return (uint32)Tween.GetHandle().Handle;
    }

    void CWorld::TweenMoveTo(uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.MoveTo(Target, Destination, Duration);
        }
    }

    void CWorld::TweenRotateTo(uint32 Tween, ECS::FEntity Target, FQuat Destination, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.RotateTo(Target, Destination, Duration);
        }
    }

    void CWorld::TweenScaleTo(uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.ScaleTo(Target, Destination, Duration);
        }
    }

    void CWorld::TweenValueTo(uint32 Tween, float From, float To, float Duration, SScriptCallback OnValue)
    {
        FTween Builder;
        if (!ResolveTween(*this, Tween, Builder))
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

    void CWorld::TweenInterval(uint32 Tween, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Interval(Duration);
        }
    }

    void CWorld::TweenCall(uint32 Tween, SScriptCallback OnCall)
    {
        FTween Builder;
        if (!ResolveTween(*this, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnCall);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnCall);
        Builder.Call([Owner] { Owner->Invoke(0); });
    }

    void CWorld::OnTweenFinished(uint32 Tween, SScriptCallback OnFinished)
    {
        FTween Builder;
        if (!ResolveTween(*this, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnFinished);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnFinished);
        Builder.OnFinished([Owner] { Owner->Invoke(0); });
    }

    void CWorld::TweenTrans(uint32 Tween, EEaseTransition Transition)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Trans(Transition);
        }
    }

    void CWorld::TweenEase(uint32 Tween, EEaseType Ease)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Ease(Ease);
        }
    }

    void CWorld::TweenDelay(uint32 Tween, float Seconds)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Delay(Seconds);
        }
    }

    void CWorld::TweenParallel(uint32 Tween)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Parallel();
        }
    }

    void CWorld::SetTweenLoops(uint32 Tween, int32 Count)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.SetLoops(Count);
        }
    }

    void CWorld::SetTweenSpeedScale(uint32 Tween, float Scale)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.SetSpeedScale(Scale);
        }
    }

    void CWorld::SetTweenPaused(uint32 Tween, bool bTweenPaused)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.SetPaused(bTweenPaused);
        }
    }

    void CWorld::KillTween(uint32 Tween)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Kill();
        }
    }

    bool CWorld::IsTweenRunning(uint32 Tween)
    {
        FTween Builder;
        return ResolveTween(*this, Tween, Builder) && Builder.IsRunning();
    }

    void CWorld::TweenMoveBy(uint32 Tween, ECS::FEntity Target, FVector3 Offset, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.MoveBy(Target, Offset, Duration);
        }
    }

    void CWorld::TweenRotateBy(uint32 Tween, ECS::FEntity Target, FQuat Delta, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.RotateBy(Target, Delta, Duration);
        }
    }

    void CWorld::TweenScaleBy(uint32 Tween, ECS::FEntity Target, FVector3 Factor, float Duration)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.ScaleBy(Target, Factor, Duration);
        }
    }

    void CWorld::TweenPunch(uint32 Tween, ECS::FEntity Target, ETweenTransformChannel Channel, FVector3 Punch, float Duration, int32 Vibrato, float Elasticity)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Punch(Target, Channel, Punch, Duration, Vibrato, Elasticity);
        }
    }

    void CWorld::TweenShake(uint32 Tween, ECS::FEntity Target, ETweenTransformChannel Channel, FVector3 Strength, float Duration, int32 Vibrato)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Shake(Target, Channel, Strength, Duration, Vibrato);
        }
    }

    void CWorld::TweenValueToVector2(uint32 Tween, FVector2 From, FVector2 To, float Duration, SScriptCallback OnValue)
    {
        AddValueByAddress(*this, Tween, From, To, Duration, OnValue);
    }

    void CWorld::TweenValueToVector3(uint32 Tween, FVector3 From, FVector3 To, float Duration, SScriptCallback OnValue)
    {
        AddValueByAddress(*this, Tween, From, To, Duration, OnValue);
    }

    void CWorld::TweenValueToVector4(uint32 Tween, FVector4 From, FVector4 To, float Duration, SScriptCallback OnValue)
    {
        AddValueByAddress(*this, Tween, From, To, Duration, OnValue);
    }

    void CWorld::TweenValueToQuat(uint32 Tween, FQuat From, FQuat To, float Duration, SScriptCallback OnValue)
    {
        AddValueByAddress(*this, Tween, From, To, Duration, OnValue);
    }

    void CWorld::TweenPropertyToFloat(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, float To, float Duration)
    {
        AddProperty(*this, Tween, Target, Component, Path, To, Duration);
    }

    void CWorld::TweenPropertyToVector2(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FVector2 To, float Duration)
    {
        AddProperty(*this, Tween, Target, Component, Path, To, Duration);
    }

    void CWorld::TweenPropertyToVector3(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FVector3 To, float Duration)
    {
        AddProperty(*this, Tween, Target, Component, Path, To, Duration);
    }

    void CWorld::TweenPropertyToVector4(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FVector4 To, float Duration)
    {
        AddProperty(*this, Tween, Target, Component, Path, To, Duration);
    }

    void CWorld::TweenPropertyToQuat(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FQuat To, float Duration)
    {
        AddProperty(*this, Tween, Target, Component, Path, To, Duration);
    }

    void CWorld::OnTweenLoop(uint32 Tween, SScriptCallback OnLoop)
    {
        FTween Builder;
        if (!ResolveTween(*this, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnLoop);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnLoop);
        Builder.OnLoop([Owner](int32 Loops) { Owner->Invoke((uint64)(uint32)Loops); });
    }

    void CWorld::OnTweenStep(uint32 Tween, SScriptCallback OnStep)
    {
        FTween Builder;
        if (!ResolveTween(*this, Tween, Builder))
        {
            Scripting::ReleaseScriptCallback(OnStep);
            return;
        }

        FTweenCallback Owner = TweenLibraryMakeCallback(OnStep);
        Builder.OnStep([Owner](int32 Step) { Owner->Invoke((uint64)(uint32)Step); });
    }

    void CWorld::TweenEaseCurve(uint32 Tween, CCurveAsset* Curve)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.EaseCurve(Curve);
        }
    }

    void CWorld::SetTweenLoopMode(uint32 Tween, ETweenLoopMode Mode)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.SetLoopMode(Mode);
        }
    }

    void CWorld::SetTweenIgnoreTimeScale(uint32 Tween, bool bIgnore)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.SetIgnoreTimeScale(bIgnore);
        }
    }

    void CWorld::CompleteTween(uint32 Tween)
    {
        FTween Builder;
        if (ResolveTween(*this, Tween, Builder))
        {
            Builder.Complete();
        }
    }

    float CWorld::GetTweenElapsed(uint32 Tween)
    {
        FTween Builder;
        return ResolveTween(*this, Tween, Builder) ? Builder.GetElapsed() : 0.0f;
    }

    float CWorld::GetTweenDuration(uint32 Tween)
    {
        FTween Builder;
        return ResolveTween(*this, Tween, Builder) ? Builder.GetDuration() : 0.0f;
    }

    float CWorld::GetTweenProgress(uint32 Tween)
    {
        FTween Builder;
        return ResolveTween(*this, Tween, Builder) ? Builder.GetProgress() : 0.0f;
    }

    int32 CWorld::GetTweenCompletedLoops(uint32 Tween)
    {
        FTween Builder;
        return ResolveTween(*this, Tween, Builder) ? Builder.GetCompletedLoops() : 0;
    }
}
