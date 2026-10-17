#pragma once

#include "World/ECS/ComponentType.h"
#include "Containers/Span.h"
#include "Containers/StaticArray.h"
#include "Containers/Vector.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/Sync.h"
#include "Memory/SmartPtr.h"

namespace Lumina::ECS
{
    class FRegistry;

    // Where a run of events of one type lives in a queue, as the bytes and how many there are.
    struct FEventRun
    {
        const uint8* Data  = nullptr;
        uint32       Count = 0;
    };

    // The unread events of one queue, oldest first. Two runs, since the unread span can straddle the frame boundary.
    struct FEventRuns
    {
        FEventRun First;
        FEventRun Second;
        uint32    Stride = 0;

        NODISCARD uint32 Num() const { return First.Count + Second.Count; }
    };

    // One event type in one world, staged per worker on send and readable from the next commit until the frame after it.
    class RUNTIME_API FEventQueue
    {
    public:

        explicit FEventQueue(FComponentTypeID InTypeID);
        ~FEventQueue();

        FEventQueue(const FEventQueue&) = delete;
        FEventQueue& operator=(const FEventQueue&) = delete;
        FEventQueue(FEventQueue&&) = delete;
        FEventQueue& operator=(FEventQueue&&) = delete;

        void Send(const void* Event);
        void SendMany(const void* Events, uint32 Count);

        // The events past Cursor, oldest first, and Cursor moves past them. Safe from any thread between sync points.
        NODISCARD FEventRuns Read(uint64& Cursor) const;
        NODISCARD uint32 NumUnread(uint64 Cursor) const;

        // Moves every staged event into the readable buffer and hands the new ones to the type's fan-out. True when any moved.
        bool Commit(FRegistry& Registry);

        // Drops the older buffer, so what was sent this frame stays readable for exactly one more.
        void Advance();

        void Clear();

        NODISCARD FComponentTypeID GetTypeID() const { return TypeID; }

    private:

        // Elements are laid out at Stride and grown by relocation, so a type that cares where it lives is still moved correctly.
        struct FBuffer
        {
            uint8* Data     = nullptr;
            uint32 Count    = 0;
            uint32 Capacity = 0;
            uint64 FirstIndex = 0;
        };

        void Reserve(FBuffer& Buffer, uint32 Wanted);
        void CopyInto(FBuffer& Buffer, const uint8* Source, uint32 Count);
        void RelocateInto(FBuffer& Buffer, FBuffer& Source);
        void DestroyAll(FBuffer& Buffer);
        void FreeBuffer(FBuffer& Buffer);

        // A script event reshaped by a reload invalidates every buffered byte, so its queue starts over at the new size.
        void RefreshLayout();
        NODISCARD uint8* At(const FBuffer& Buffer, uint32 Index) const { return Buffer.Data + (size_t)Index * Stride; }

        const FComponentTypeInfo& Info;
        FComponentTypeID          TypeID;
        uint32                    Stride;

        // Buffers[Newer] takes commits, and the other holds the frame before it.
        FBuffer                   Buffers[2];
        uint32                    Newer = 1;
        uint64                    NextIndex = 0;

        // One per worker slot, so a send never contends.
        TVector<FBuffer>          Staging;

        // Lets a commit skip a queue nothing sent to, without walking every slot.
        TAtomic<bool>             bStaged = false;
    };

    // Every event queue one world owns, each created the first time anything names its type.
    class RUNTIME_API FEventQueues
    {
    public:

        explicit FEventQueues(FRegistry& InRegistry);
        ~FEventQueues();

        FEventQueues(const FEventQueues&) = delete;
        FEventQueues& operator=(const FEventQueues&) = delete;
        FEventQueues(FEventQueues&&) = delete;
        FEventQueues& operator=(FEventQueues&&) = delete;

        NODISCARD FEventQueue& Get(FComponentTypeID TypeID);
        NODISCARD FEventQueue* Find(FComponentTypeID TypeID) const;

        // Commits every queue, again while a fan-out sends more, so one sync point delivers a whole chain. True when anything moved.
        bool Commit();

        void Advance();
        void Clear();

        // Someone consumes this type, so a producer that filters by listeners has to keep filling it.
        void AddInterest(FComponentTypeID TypeID);
        NODISCARD bool HasInterest(FComponentTypeID TypeID) const;

    private:

        FRegistry&                                              Registry;
        TArray<TAtomic<FEventQueue*>, MaxComponentTypes>        ByTypeID;
        TArray<TAtomic<bool>, MaxComponentTypes>                Interest;
        TVector<TUniquePtr<FEventQueue>>                        Owned;
        TVector<FEventQueue*>                                   CommitScratch;
        FMutex                                                  CreateLock;
    };

    // The queue type an event goes by, its reflected struct or the name a script registered it under. Invalid when neither.
    RUNTIME_API FComponentTypeID FindEventType(FName Name);

    // A script event's queue type, laid out from what the script reports and laid out again when it reshapes.
    RUNTIME_API FComponentTypeID AcquireScriptEventType(FName Name, uint32 Size, uint32 Alignment);

    // How far one reader has read a queue, kept by whoever reads it so every reader sees each event exactly once.
    template<typename T>
    struct TEventReader
    {
        uint64 Cursor = 0;
    };

    // A typed view over unread events, iterated oldest first.
    template<typename T>
    struct TEventRange
    {
        TSpan<const T> First;
        TSpan<const T> Second;

        struct FIterator
        {
            const TEventRange* Range = nullptr;
            uint32             Index = 0;

            const T& operator*() const
            {
                const uint32 FirstNum = (uint32)Range->First.size();
                return Index < FirstNum ? Range->First[Index] : Range->Second[Index - FirstNum];
            }
            FIterator& operator++() { ++Index; return *this; }
            bool operator!=(const FIterator& Other) const { return Index != Other.Index; }
        };

        NODISCARD FIterator begin() const { return FIterator{ this, 0 }; }
        NODISCARD FIterator end() const { return FIterator{ this, Num() }; }
        NODISCARD uint32 Num() const { return (uint32)(First.size() + Second.size()); }
        NODISCARD bool IsEmpty() const { return Num() == 0; }
    };

    template<typename T>
    NODISCARD TEventRange<T> MakeEventRange(const FEventRuns& Runs)
    {
        TEventRange<T> Range;
        Range.First  = TSpan<const T>(reinterpret_cast<const T*>(Runs.First.Data), Runs.First.Count);
        Range.Second = TSpan<const T>(reinterpret_cast<const T*>(Runs.Second.Data), Runs.Second.Count);
        return Range;
    }

    template<typename T>
    void SendEvent(FEventQueues& Queues, const T& Event)
    {
        Queues.Get(GetComponentTypeID<T>()).Send(&Event);
    }

    template<typename T>
    void SendEvents(FEventQueues& Queues, TSpan<const T> Events)
    {
        if (!Events.empty())
        {
            Queues.Get(GetComponentTypeID<T>()).SendMany(Events.data(), (uint32)Events.size());
        }
    }

    // Reading marks interest, so a reader no system declared still gets a producer to fill its queue from the next frame on.
    template<typename T>
    NODISCARD TEventRange<T> ReadEvents(FEventQueues& Queues, TEventReader<T>& Reader)
    {
        const FComponentTypeID TypeID = GetComponentTypeID<T>();
        Queues.AddInterest(TypeID);
        return MakeEventRange<T>(Queues.Get(TypeID).Read(Reader.Cursor));
    }

    template<typename T>
    NODISCARD bool HasUnreadEvents(const FEventQueues& Queues, const TEventReader<T>& Reader)
    {
        const FEventQueue* Queue = Queues.Find(GetComponentTypeID<T>());
        return Queue != nullptr && Queue->NumUnread(Reader.Cursor) != 0;
    }
}
