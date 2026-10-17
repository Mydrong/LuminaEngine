#include "RuntimePCH.h"
#include "World/Entity/Components/PerceptionComponent.h"
#include "World/Entity/Components/PhysicsComponent.h"
#include "World/Entity/Events/CollisionEvent.h"
#include "World/Entity/Events/PerceptionEvent.h"

namespace Lumina
{
    namespace
    {
        template<typename TComponent>
        TComponent* FindListening(ECS::FRegistry& Registry, ECS::FEntity Entity)
        {
            return Registry.IsValid(Entity) ? Registry.TryGet<TComponent>(Entity) : nullptr;
        }
    }

    void TEventFanOut<SCollisionEvent>::Deliver(ECS::FRegistry& Registry, const SCollisionEvent& Event)
    {
        SRigidBodyComponent* Body = FindListening<SRigidBodyComponent>(Registry, Event.Entity);
        if (Body == nullptr)
        {
            return;
        }

        TScriptDelegate<SCollisionEvent>* Delegate = nullptr;
        switch (Event.Kind)
        {
        case ECollisionEventKind::ContactBegin: Delegate = &Body->OnContactBegin; break;
        case ECollisionEventKind::ContactEnd:   Delegate = &Body->OnContactEnd;   break;
        case ECollisionEventKind::OverlapBegin: Delegate = &Body->OnOverlapBegin; break;
        case ECollisionEventKind::OverlapEnd:   Delegate = &Body->OnOverlapEnd;   break;
        }

        if (Delegate != nullptr && Delegate->IsBound())
        {
            Delegate->Broadcast(Event);
        }
    }

    void TEventFanOut<SBodyActivationEvent>::Deliver(ECS::FRegistry& Registry, const SBodyActivationEvent& Event)
    {
        if (SRigidBodyComponent* Body = FindListening<SRigidBodyComponent>(Registry, Event.Entity))
        {
            FScriptDelegate& Delegate = Event.bAwake ? Body->OnWake : Body->OnSleep;
            if (Delegate.IsBound())
            {
                Delegate.Broadcast();
            }
        }
    }

    void TEventFanOut<SPerceptionEvent>::Deliver(ECS::FRegistry& Registry, const SPerceptionEvent& Event)
    {
        if (SPerceptionComponent* Perception = FindListening<SPerceptionComponent>(Registry, Event.Perceiver))
        {
            TScriptDelegate<SPerceptionEvent>& Delegate = Event.bPerceived ? Perception->OnTargetPerceived : Perception->OnTargetLost;
            if (Delegate.IsBound())
            {
                Delegate.Broadcast(Event);
            }
        }
    }
}
