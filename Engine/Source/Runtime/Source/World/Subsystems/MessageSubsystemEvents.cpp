#include "RuntimePCH.h"
#include "World/Subsystems/MessageSubsystem.h"

#include "World/ECS/EventQueue.h"
#include "World/World.h"

namespace Lumina
{
    // EventRuns in LuminaSharp/World/Events.cs reads this layout field for field.
    static_assert(sizeof(ECS::FEventRuns) == 40 && offsetof(ECS::FEventRuns, Second) == 16 && offsetof(ECS::FEventRuns, Stride) == 32);

    namespace
    {
        ECS::FEventQueue* QueueFor(CWorld* World, uint32 TypeId)
        {
            if (World == nullptr || TypeId >= ECS::MaxComponentTypes)
            {
                return nullptr;
            }
            return &World->GetEvents().Get((ECS::FComponentTypeID)TypeId);
        }
    }

    uint32 CMessageSubsystem::ResolveEventType(FName Name, uint32 Size, uint32 Alignment)
    {
        const ECS::FComponentTypeID Found = ECS::FindEventType(Name);
        if (Found != ECS::InvalidComponentTypeID)
        {
            return Found;
        }
        return ECS::AcquireScriptEventType(Name, Size, Alignment);
    }

    void CMessageSubsystem::SendEvents(uint32 TypeId, void* Events, uint32 Count)
    {
        if (ECS::FEventQueue* Queue = QueueFor(GetWorld(), TypeId); Queue != nullptr && Events != nullptr)
        {
            Queue->SendMany(Events, Count);
        }
    }

    void CMessageSubsystem::ReadEvents(uint32 TypeId, void* Cursor, void* Runs)
    {
        CWorld* World = GetWorld();
        ECS::FEventQueue* Queue = QueueFor(World, TypeId);
        if (Queue == nullptr || Cursor == nullptr || Runs == nullptr)
        {
            return;
        }

        World->GetEvents().AddInterest((ECS::FComponentTypeID)TypeId);
        *static_cast<ECS::FEventRuns*>(Runs) = Queue->Read(*static_cast<uint64*>(Cursor));
    }

    uint32 CMessageSubsystem::NumUnreadEvents(uint32 TypeId, uint64 Cursor)
    {
        CWorld* World = GetWorld();
        const ECS::FEventQueue* Queue = World != nullptr ? World->GetEvents().Find((ECS::FComponentTypeID)TypeId) : nullptr;
        return Queue != nullptr ? Queue->NumUnread(Cursor) : 0u;
    }
}
