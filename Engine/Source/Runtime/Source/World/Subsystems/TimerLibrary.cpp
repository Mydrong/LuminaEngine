#include "RuntimePCH.h"

#include "TimerLibrary.h"

#include "Containers/HashTable.h"
#include "Containers/Vector.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Memory/SmartPtr.h"
#include "World/Subsystems/TimerManager.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // Tracked so a generation unload stops the timer itself, not just the callback it can no longer run.
        struct FManagedTimer
        {
            FScriptCallbackOwner   Callback;
            TWeakObjectPtr<CWorld> World;
            FTimerHandle           Handle;

            FManagedTimer(SScriptCallback InCallback, CWorld* InWorld)
                : Callback(InCallback)
                , World(InWorld)
            {
                Live().insert(this);
            }

            ~FManagedTimer() { Live().erase(this); }

            LE_NO_COPYMOVE(FManagedTimer);

            static THashSet<FManagedTimer*>& Live()
            {
                static THashSet<FManagedTimer*> Set;
                return Set;
            }
        };

        // Shared, since a looping timer moves its callback out and back on every fire.
        FTimerManager::FTimerCallback MakeCallback(const TSharedPtr<FManagedTimer>& Timer)
        {
            return [Timer]() { Timer->Callback.Invoke(0); };
        }

        uint32 Schedule(CWorld* World, ECS::FEntity Owner, bool bHasOwner, float Rate, bool bLoop,
            float FirstDelay, SScriptCallback Callback)
        {
            if (World == nullptr || !Callback.IsBound())
            {
                Scripting::ReleaseScriptCallback(Callback);
                return (uint32)ECS::NullEntity;
            }

            TSharedPtr<FManagedTimer> Timer = MakeShared<FManagedTimer>(Callback, World);

            const FTimerHandle Handle = bHasOwner
                ? World->GetTimerManager().SetTimerForEntity(Owner, Rate, MakeCallback(Timer), bLoop, FirstDelay)
                : World->GetTimerManager().SetTimer(Rate, MakeCallback(Timer), bLoop, FirstDelay);

            if (Handle.Handle == ECS::NullEntity)
            {
                return (uint32)ECS::NullEntity;
            }

            Timer->Handle = Handle;
            return (uint32)Handle.Handle;
        }
    }

    uint32 CTimerLibrary::SetTimer(CWorld* World, float Rate, SScriptCallback OnFire, bool bLoop, float FirstDelay)
    {
        return Schedule(World, ECS::NullEntity, false, Rate, bLoop, FirstDelay, OnFire);
    }

    uint32 CTimerLibrary::SetTimerForEntity(CWorld* World, ECS::FEntity Owner, float Rate, SScriptCallback OnFire,
        bool bLoop, float FirstDelay)
    {
        return Schedule(World, Owner, true, Rate, bLoop, FirstDelay, OnFire);
    }

    void CTimerLibrary::Clear(CWorld* World, uint32 Timer)
    {
        if (World != nullptr)
        {
            FTimerHandle Handle{ ECS::FEntity::FromPacked(Timer) };
            World->GetTimerManager().ClearTimer(Handle);
        }
    }

    bool CTimerLibrary::IsActive(CWorld* World, uint32 Timer)
    {
        return World != nullptr
            && World->GetTimerManager().IsTimerActive(FTimerHandle{ ECS::FEntity::FromPacked(Timer) });
    }

    float CTimerLibrary::GetRemaining(CWorld* World, uint32 Timer)
    {
        return World != nullptr
            ? World->GetTimerManager().GetTimerRemaining(FTimerHandle{ ECS::FEntity::FromPacked(Timer) })
            : 0.0f;
    }

    float CTimerLibrary::GetElapsed(CWorld* World, uint32 Timer)
    {
        return World != nullptr
            ? World->GetTimerManager().GetTimerElapsed(FTimerHandle{ ECS::FEntity::FromPacked(Timer) })
            : 0.0f;
    }

    void CTimerLibrary::SetPaused(CWorld* World, uint32 Timer, bool bPaused)
    {
        if (World != nullptr)
        {
            World->GetTimerManager().SetTimerPaused(FTimerHandle{ ECS::FEntity::FromPacked(Timer) }, bPaused);
        }
    }

    void CTimerLibrary::ClearAllManaged()
    {
        // Snapshotted, since clearing a timer destroys its entry and mutates the set.
        TVector<FManagedTimer*> Snapshot;
        Snapshot.reserve(FManagedTimer::Live().size());
        for (FManagedTimer* Timer : FManagedTimer::Live())
        {
            Snapshot.push_back(Timer);
        }

        for (FManagedTimer* Timer : Snapshot)
        {
            if (CWorld* World = Timer->World.Get())
            {
                World->GetTimerManager().ClearTimer(Timer->Handle);
            }
        }
    }
}
