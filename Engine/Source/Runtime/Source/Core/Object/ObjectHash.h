#pragma once
#include "Containers/Name.h"
#include "Containers/Vector.h"
#include "Core/Singleton/Singleton.h"
#include "Core/Threading/Thread.h"
#include "GUID/GUID.h"


namespace Lumina
{
    class CClass;
    class CPackage;
    class CObjectBase;
}

namespace Lumina
{
    // Open-addressed multiset of objects under a hash of a key they carry, so the key is read back from the object instead of stored.
    class FObjectIndexTable
    {
    public:

        struct FSlot
        {
            CObjectBase* Object = nullptr;
            uint64       Hash   = 0;
        };

        void Insert(CObjectBase* Object, uint64 Hash);

        // False when the object was not in the table under that hash.
        bool Remove(const CObjectBase* Object, uint64 Hash);

        // Visits every object stored under Hash until Visit returns true, and returns that object.
        template<typename TVisit>
        CObjectBase* FindFirst(uint64 Hash, TVisit&& Visit) const
        {
            if (Slots.empty())
            {
                return nullptr;
            }

            const uint64 Mask = Slots.size() - 1;
            for (uint64 Index = Hash & Mask; Slots[Index].Object != nullptr; Index = (Index + 1) & Mask)
            {
                if (Slots[Index].Hash == Hash && Visit(Slots[Index].Object))
                {
                    return Slots[Index].Object;
                }
            }
            return nullptr;
        }

        void Clear();

        size_t GetAllocatedBytes() const { return Slots.capacity() * sizeof(FSlot); }

    private:

        void Grow();

        TVector<FSlot> Slots;
        uint32         Count = 0;
    };

    class FObjectHashTables : public TSingleton<FObjectHashTables>
    {
    public:

        void AddObject(CObjectBase* Object);

        void RemoveObject(CObjectBase* Object);

        CObjectBase* FindObject(const FGuid& GUID);
        CObjectBase* FindObject(const FName& Name, CClass* Class);
        CObjectBase* FindObject(const FName& Name, const CPackage* Package);

        void Clear();

    private:

        // Loader threads register objects by the thousand, so one table behind one lock stalled every lookup while it regrew.
        static constexpr uint32 ShardBits = 6;
        static constexpr uint32 ShardCount = 1u << ShardBits;

        struct alignas(64) FShard
        {
            mutable FSharedMutex Mutex;
            FObjectIndexTable    Objects;
        };

        // The shard comes from the top bits and the slot from the bottom, so the two never correlate.
        static uint32 ShardOf(uint64 Hash) noexcept { return static_cast<uint32>(Hash >> (64 - ShardBits)); }

        static uint64 HashOf(const FGuid& GUID) noexcept { return Containers::MixHash64(GetTypeHash(GUID)); }
        static uint64 HashOf(const FName& Name) noexcept { return GetTypeHash(Name); }

        FShard GuidShards[ShardCount];
        FShard NameShards[ShardCount];
    };
}
