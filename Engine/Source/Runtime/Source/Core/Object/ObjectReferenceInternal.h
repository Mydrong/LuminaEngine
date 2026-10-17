#pragma once

#include "Core/LuminaMacros.h"
#include "Core/Threading/Atomic.h"
#include "ObjectHandle.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class CObjectBase;
}

namespace Lumina::Object::Internal
{
    struct FCObjectEntry
    {
        FCObjectEntry() = default;

        static constexpr int32 DestroyClaimed = INT32_MIN;

        CObjectBase* Object = nullptr;
        TAtomic<int32> Generation{0};
        TAtomic<int32> StrongRefCount{0};

        FCObjectEntry(FCObjectEntry&&) = delete;
        ~FCObjectEntry() = default;
        FCObjectEntry(const FCObjectEntry&) = delete;
        FCObjectEntry& operator=(FCObjectEntry&&) = delete;
        FCObjectEntry& operator=(const FCObjectEntry&) = delete;

        FORCEINLINE CObjectBase* GetObj() const
        {
            return Object;
        }

        FORCEINLINE void SetObj(CObjectBase* InObject)
        {
            Object = InObject;
        }

        bool AddStrongRefIfAlive()
        {
            int32 PrevCount = StrongRefCount.load(std::memory_order_acquire);
            do
            {
                if (PrevCount < 0)
                {
                    return false;
                }
            }
            while (!StrongRefCount.compare_exchange_weak(PrevCount, PrevCount + 1,
                std::memory_order_acq_rel, std::memory_order_acquire));
            return true;
        }

        bool ReleaseStrongRef(int32& OutNewCount)
        {
            int32 PrevCount = StrongRefCount.load(std::memory_order_relaxed);
            do
            {
                if (PrevCount <= 0)
                {
                    OutNewCount = 0;
                    return false;
                }
            }
            while (!StrongRefCount.compare_exchange_weak(PrevCount, PrevCount - 1,
                std::memory_order_acq_rel, std::memory_order_relaxed));
            OutNewCount = PrevCount - 1;
            return true;
        }

        bool TryClaimDestroyIfUnreferenced()
        {
            int32 Expected = 0;
            return StrongRefCount.compare_exchange_strong(Expected, DestroyClaimed,
                std::memory_order_acq_rel, std::memory_order_relaxed);
        }

        bool TryClaimDestroyForced()
        {
            int32 PrevCount = StrongRefCount.load(std::memory_order_relaxed);
            do
            {
                if (PrevCount == DestroyClaimed)
                {
                    return false;
                }
            }
            while (!StrongRefCount.compare_exchange_weak(PrevCount, DestroyClaimed,
                std::memory_order_acq_rel, std::memory_order_relaxed));
            return true;
        }

        int32 GetStrongRefCount() const
        {
            const int32 Count = StrongRefCount.load(std::memory_order_relaxed);
            return Count > 0 ? Count : 0;
        }

        int32 GetGeneration() const
        {
            return Generation.load(std::memory_order_acquire);
        }

        void IncrementGeneration()
        {
            Generation.fetch_add(1, std::memory_order_release);
        }

        bool IsReferenced() const
        {
            return StrongRefCount.load(std::memory_order_relaxed) > 0;
        }

        void ResetRefCount()
        {
            StrongRefCount.store(0, std::memory_order_relaxed);
        }
    };

    RUNTIME_API CObjectBase* ResolveHandle(const FObjectHandle& Handle);
    RUNTIME_API FObjectHandle GetHandleByObject(const CObjectBase* Object);
    RUNTIME_API FCObjectEntry* GetEntry(const CObjectBase* Object);
    RUNTIME_API bool ReleaseStrongRefEntry(FCObjectEntry* Entry, const CObjectBase* Expected);
    RUNTIME_API CObjectBase* TryAddStrongRef(const FObjectHandle& Handle);
}
