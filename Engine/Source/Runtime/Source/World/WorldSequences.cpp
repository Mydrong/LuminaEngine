#include "RuntimePCH.h"
#include "World/ECS/Registry.h"

#include "World/World.h"
#include "World/Entity/Components/SequencePlayerComponent.h"
#include "World/Entity/Systems/CameraSystem.h"

namespace Lumina
{
    namespace
    {
        struct FSequenceEventCursor
        {
            TVector<SSequenceFiredEvent> Pending;
            size_t                       Next = 0;
            SSequenceFiredEvent          Current;
        };

        FSequenceEventCursor& EventCursor(CWorld& World)
        {
            ECS::FRegistry& Registry = World.GetEntityRegistry();
            if (FSequenceEventCursor* Cursor = Registry.Ctx().Find<FSequenceEventCursor>())
            {
                return *Cursor;
            }
            return Registry.Ctx().Emplace<FSequenceEventCursor>();
        }
    }

    ECS::FEntity CWorld::PlaySequence(CSequence* Sequence, bool bLoop, float PlayRate)
    {
        if (Sequence == nullptr)
        {
            return ECS::NullEntity;
        }

        const ECS::FEntity Entity = ConstructEntity("SequencePlayer");
        SSequencePlayerComponent& Player = GetOrEmplaceComponent<SSequencePlayerComponent>(Entity);
        Player.Sequence = Sequence;
        Player.bLoop = bLoop;
        Player.PlayRate = PlayRate > 0.0f ? PlayRate : 1.0f;
        Player.Play(true);
        return Entity;
    }

    void CWorld::StopSequence(ECS::FEntity Player)
    {
        if (!IsValidEntity(Player))
        {
            return;
        }

        if (SSequencePlayerComponent* Component = TryGetComponent<SSequencePlayerComponent>(Player))
        {
            Component->Instance.Release(this, Component->FinishAction == ESequenceFinishAction::Restore);
            Component->bPlaying = false;
        }
        DestroyEntity(Player);
    }

    bool CWorld::IsSequencePlaying(ECS::FEntity Player)
    {
        if (!IsValidEntity(Player))
        {
            return false;
        }

        const SSequencePlayerComponent* Component = TryGetComponent<SSequencePlayerComponent>(Player);
        return Component != nullptr && Component->bPlaying;
    }

    void CWorld::ConsumeSequenceEvents(TVector<SSequenceFiredEvent>& OutEvents)
    {
        auto Players = View<SSequencePlayerComponent>();
        for (ECS::FEntity Entity : Players)
        {
            TVector<SSequenceFiredEvent>& Fired = Players.Get<SSequencePlayerComponent>(Entity).Instance.FiredEvents;
            for (SSequenceFiredEvent& Event : Fired)
            {
                OutEvents.push_back(Move(Event));
            }
            Fired.clear();
        }
    }

    bool CWorld::NextSequenceEvent()
    {
        FSequenceEventCursor& Cursor = EventCursor(*this);
        if (Cursor.Next >= Cursor.Pending.size())
        {
            Cursor.Pending.clear();
            Cursor.Next = 0;
            ConsumeSequenceEvents(Cursor.Pending);
        }
        if (Cursor.Next >= Cursor.Pending.size())
        {
            Cursor.Current = SSequenceFiredEvent();
            return false;
        }

        Cursor.Current = Move(Cursor.Pending[Cursor.Next++]);
        return true;
    }

    FString CWorld::GetSequenceEventName()
    {
        return FString(EventCursor(*this).Current.Name.ToString().c_str());
    }

    FString CWorld::GetSequenceEventPayload()
    {
        return EventCursor(*this).Current.Payload;
    }

    ECS::FEntity CWorld::GetSequenceEventTarget()
    {
        return EventCursor(*this).Current.Target;
    }

    float CWorld::GetSequenceEventTime()
    {
        return EventCursor(*this).Current.Time;
    }

    bool CWorld::IsAnySequencePlaying()
    {
        auto Players = View<SSequencePlayerComponent>();
        for (ECS::FEntity Entity : Players)
        {
            if (Players.Get<SSequencePlayerComponent>(Entity).bPlaying)
            {
                return true;
            }
        }
        return false;
    }

    void CWorld::SetCinematicOverlay(float Fade, FVector3 Color, float LetterboxAspect)
    {
        {
            SCameraSystem::SetCinematicOverlay(EntityRegistry, Fade, Color, LetterboxAspect);
        }
    }
}
