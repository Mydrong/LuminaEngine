#pragma once

#include "World/ECS/EventQueue.h"
#include "World/ECS/Registry.h"

namespace Lumina
{
    // Specialized beside an event, with a static Deliver(ECS::FRegistry&, const T&), to hand each committed one to entity delegates.
    template<typename T>
    struct TEventFanOut;

    template<typename T>
    struct TMemberPointerClass;

    template<typename TMember, typename TClass>
    struct TMemberPointerClass<TMember TClass::*>
    {
        using Type = TClass;
    };

    // The usual fan-out, Delegate on the component of the entity that EntityField names.
    template<typename TEvent, auto Delegate, auto EntityField = &TEvent::Entity>
    struct TEntityDelegateFanOut
    {
        static void Deliver(ECS::FRegistry& Registry, const TEvent& Event)
        {
            using TComponent = typename TMemberPointerClass<decltype(Delegate)>::Type;

            const ECS::FEntity Entity = Event.*EntityField;
            TComponent* Component = Registry.IsValid(Entity) ? Registry.TryGet<TComponent>(Entity) : nullptr;
            if (Component != nullptr && (Component->*Delegate).IsBound())
            {
                (Component->*Delegate).Broadcast(Event);
            }
        }
    };

    namespace Meta
    {
        template<typename T>
        concept CHasEventFanOut = requires(ECS::FRegistry& Registry, const T& Event)
        {
            TEventFanOut<T>::Deliver(Registry, Event);
        };

        template<typename T>
        void FanOutEvents(ECS::FRegistry& Registry, const void* Events, uint32 Count)
        {
            const T* Typed = static_cast<const T*>(Events);
            for (uint32 Index = 0; Index < Count; ++Index)
            {
                TEventFanOut<T>::Deliver(Registry, Typed[Index]);
            }
        }

        // Emitted for every REFLECT(Event), so a script can name the event and its queue knows where to fan out.
        template<typename T>
        void RegisterEventMeta()
        {
            ECS::FComponentTypeRegistry& Types = ECS::FComponentTypeRegistry::Get();
            const ECS::FComponentTypeID TypeID = ECS::GetComponentTypeID<T>();
            Types.BindStruct(TypeID, T::StaticStruct());

            if constexpr (CHasEventFanOut<T>)
            {
                Types.SetEventFanOut(TypeID, &FanOutEvents<T>);
            }
        }
    }
}
