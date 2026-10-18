#include "RuntimePCH.h"

#include "World/ECS/EventQueue.h"
#include "World/World.h"

namespace Lumina
{
    // EventRuns in LuminaSharp/World/Events.cs reads this layout field for field.
    static_assert(sizeof(ECS::FEventRuns) == 40 && offsetof(ECS::FEventRuns, Second) == 16 && offsetof(ECS::FEventRuns, Stride) == 32);

    namespace
    {
        ECS::FEventQueue* QueueFor(CWorld& World, uint32 TypeId)
        {
            if (TypeId >= ECS::MaxComponentTypes)
            {
                return nullptr;
            }
            return &World.GetEvents().Get((ECS::FComponentTypeID)TypeId);
        }
    }

    uint32 CWorld::ResolveEventType(FName Name, uint32 Size, uint32 Alignment)
    {
        const ECS::FComponentTypeID Found = ECS::FindEventType(Name);
        if (Found != ECS::InvalidComponentTypeID)
        {
            return Found;
        }
        return ECS::AcquireScriptEventType(Name, Size, Alignment);
    }

    void CWorld::SendEvents(uint32 TypeId, void* Events, uint32 Count)
    {
        if (ECS::FEventQueue* Queue = QueueFor(*this, TypeId); Queue != nullptr && Events != nullptr)
        {
            Queue->SendMany(Events, Count);
        }
    }

    void CWorld::ReadEvents(uint32 TypeId, void* Cursor, void* Runs)
    {
        ECS::FEventQueue* Queue = QueueFor(*this, TypeId);
        if (Queue == nullptr || Cursor == nullptr || Runs == nullptr)
        {
            return;
        }

        GetEvents().AddInterest((ECS::FComponentTypeID)TypeId);
        *static_cast<ECS::FEventRuns*>(Runs) = Queue->Read(*static_cast<uint64*>(Cursor));
    }

    uint32 CWorld::NumUnreadEvents(uint32 TypeId, uint64 Cursor) const
    {
        const ECS::FEventQueue* Queue = GetEvents().Find((ECS::FComponentTypeID)TypeId);
        return Queue != nullptr ? Queue->NumUnread(Cursor) : 0u;
    }
}
