#include "RuntimePCH.h"
#include "Coroutine.h"

#include "ThreadedCallback.h"
#include "Core/Threading/Thread.h"
#include "Scheduler/JobScheduler.h"
#include "World/World.h"
#include "World/Subsystems/CoroutineManager.h"

namespace Lumina::CoroDetail
{
    void FCoroutineState::Release()
    {
        if (RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }

    void FCoroutineState::Cancel()
    {
        bCancelRequested.store(true, std::memory_order_release);

        // Copied out under the lock, so a child that finishes meanwhile is still alive when told.
        TVector<FStateRef> Children;
        {
            TScopeLock Guard(Lock);
            for (FCoroutineState* Child : AwaitedChildren)
            {
                Children.emplace_back(Child);
            }
        }
        const bool bForce = bForced.load(std::memory_order_acquire);
        for (FStateRef& Child : Children)
        {
            if (bForce)
            {
                Child->ForceCancel();
            }
            else
            {
                Child->Cancel();
            }
        }
    }

    void FCoroutineState::ForceCancel()
    {
        bForced.store(true, std::memory_order_release);
        Cancel();
    }

    bool FCoroutineState::ShouldDieAtSuspension() const
    {
        return bForced.load(std::memory_order_acquire) || (IsCancelRequested() && GuardCount == 0);
    }

    bool FCoroutineState::TryContinueWith(TMoveOnlyFunction<void()>&& Callback)
    {
        TScopeLock Guard(Lock);
        if (IsDone())
        {
            return false;
        }
        Continuations.push_back(std::move(Callback));
        return true;
    }

    void FCoroutineState::ContinueWith(TMoveOnlyFunction<void()>&& Callback)
    {
        if (!TryContinueWith(std::move(Callback)))
        {
            Callback();
        }
    }

    void FCoroutineState::Resume()
    {
        if (ShouldDieAtSuspension())
        {
            DestroyAsCanceled();
            return;
        }
        ASSERT(Handle, "Resumed a coroutine that has already ended.");
        Handle.resume();
    }

    void FCoroutineState::DestroyAsCanceled()
    {
        FStateRef Keep(this);
        std::coroutine_handle<> Frame = Handle;
        if (!Frame)
        {
            return;
        }
        Handle = {};
        bCancelRequested.store(true, std::memory_order_release);
        bDestroyingForCancel = true;
        Frame.destroy();
        Complete(EStatus::Canceled);
    }

    bool FCoroutineState::ClaimWait(uint32 Serial)
    {
        uint32 Expected = Serial;
        return WaitSerial.compare_exchange_strong(Expected, Serial + 1, std::memory_order_acq_rel);
    }

    void FCoroutineState::BindWorld(CWorld* InWorld)
    {
        if (InWorld == nullptr || World != nullptr)
        {
            return;
        }
        World = InWorld;
        World->GetCoroutineManager().Track(*this);
    }

    void FCoroutineState::BindOwner(CObject* InOwner)
    {
        if (InOwner != nullptr && !bHasOwner)
        {
            Owner     = InOwner;
            bHasOwner = true;
        }
    }

    void FCoroutineState::AwaitChildren(TVector<FCoroutineState*>&& InChildren)
    {
        {
            TScopeLock Guard(Lock);
            AwaitedChildren = std::move(InChildren);
        }

        // A cancel that landed before the children were known still has to reach them.
        if (IsCancelRequested())
        {
            Cancel();
        }
    }

    void FCoroutineState::ClearAwaitedChildren()
    {
        TScopeLock Guard(Lock);
        AwaitedChildren.clear();
    }

    void FCoroutineState::FinishSucceeded()
    {
        Complete(EStatus::Succeeded);
    }

    void FCoroutineState::Complete(EStatus Final)
    {
        TVector<TMoveOnlyFunction<void()>> Run;
        {
            TScopeLock Guard(Lock);
            Status.store(Final, std::memory_order_release);
            Run = std::move(Continuations);
            Continuations.clear();
            AwaitedChildren.clear();
        }

        if (World != nullptr)
        {
            World->GetCoroutineManager().Untrack(*this);
            World = nullptr;
        }

        for (TMoveOnlyFunction<void()>& Callback : Run)
        {
            Callback();
        }
    }

    void SuspendLatent(FCoroutineState& State, FLatentWaitSpec&& Spec, uint32 Serial)
    {
        ASSERT(State.GetWorld() != nullptr, "A latent wait needs a world. Pass a CWorld*, or an object with GetWorld(), as the coroutine's first parameter.");
        State.GetWorld()->GetCoroutineManager().Suspend(State, std::move(Spec), Serial);
    }

    namespace
    {
        void ResumeTargetThunk(void* Arg, uint32)
        {
            FResumeTarget* Target = static_cast<FResumeTarget*>(Arg);
            FResumeTarget Local = std::move(*Target);
            delete Target;
            Local();
        }
    }

    void ScheduleOnTask(FResumeTarget Target, ETaskPriority Priority)
    {
        Jobs::RunJob(&ResumeTargetThunk, new FResumeTarget(std::move(Target)), ToJobPriority(Priority), nullptr, "Coro");
    }

    void ScheduleOnGameThread(FResumeTarget Target)
    {
        MainThread::Enqueue([Target = std::move(Target)]() { Target(); });
    }

    bool IsOnGameThread()
    {
        return Threading::IsMainThread();
    }
}
