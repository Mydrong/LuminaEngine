#include "RuntimePCH.h"

#include "World/World.h"
#include "World/Subsystems/CoroutineManager.h"

namespace Lumina
{
    namespace
    {
        // Owns the callback's handle, so a wait dropped without firing still frees it.
        struct FOneShotCallback
        {
            SScriptCallback Callback;

            explicit FOneShotCallback(SScriptCallback In) : Callback(In) {}
            FOneShotCallback(FOneShotCallback&& Other) noexcept : Callback(Other.Callback) { Other.Callback = {}; }
            FOneShotCallback(const FOneShotCallback&) = delete;
            FOneShotCallback& operator=(const FOneShotCallback&) = delete;
            FOneShotCallback& operator=(FOneShotCallback&&) = delete;
            ~FOneShotCallback() { Scripting::ReleaseScriptCallback(Callback); }

            void Fire(bool bCompleted)
            {
                SScriptCallback Taken = Callback;
                Callback = {};
                Scripting::InvokeScriptCallback(Taken, bCompleted ? 1 : 0);
            }
        };

        uint64 Schedule(CWorld& World, CoroDetail::FLatentWaitSpec&& Spec, SScriptCallback OnDone)
        {
            if (!OnDone.IsBound())
            {
                Scripting::ReleaseScriptCallback(OnDone);
                return 0;
            }
            return World.GetCoroutineManager().AddWait(std::move(Spec), [Owned = FOneShotCallback(OnDone)](bool bCompleted) mutable
            {
                Owned.Fire(bCompleted);
            });
        }
    }

    uint64 CWorld::WaitTicks(int32 Ticks, SScriptCallback OnDone)
    {
        CoroDetail::FLatentWaitSpec Spec;
        Spec.Kind  = CoroDetail::FLatentWaitSpec::EKind::Ticks;
        Spec.Ticks = Ticks;
        return Schedule(*this, std::move(Spec), OnDone);
    }

    uint64 CWorld::WaitSeconds(double Seconds, ECoroutineClock Clock, SScriptCallback OnDone)
    {
        CoroDetail::FLatentWaitSpec Spec;
        Spec.Kind    = CoroDetail::FLatentWaitSpec::EKind::Time;
        Spec.Clock   = Clock;
        Spec.Seconds = Seconds;
        return Schedule(*this, std::move(Spec), OnDone);
    }

    void CWorld::CancelWait(uint64 Wait)
    {
        if (Wait != 0)
        {
            GetCoroutineManager().CancelWait(Wait);
        }
    }

    int32 CWorld::GetPendingWaitCount() const
    {
        return GetCoroutineManager().NumWaiting();
    }
}
