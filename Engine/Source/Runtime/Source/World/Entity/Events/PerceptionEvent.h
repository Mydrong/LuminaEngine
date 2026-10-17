#pragma once

#include "World/ECS/Registry.h"
#include "World/ECS/EventMeta.h"

#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Math.h"
#include "AI/Perception/PerceptionTypes.h"
#include "PerceptionEvent.generated.h"

namespace Lumina
{
    // A perceiver gained or lost a target, queued for systems and fanned out to its perception component's delegates.
    REFLECT(Event)
    struct RUNTIME_API SPerceptionEvent
    {
        GENERATED_BODY()

        /** This perceiving entity. */
        PROPERTY(Hidden)
        ECS::FEntity Perceiver = ECS::NullEntity;

        /** The perceived (or lost) entity. */
        PROPERTY(Hidden)
        ECS::FEntity Target = ECS::NullEntity;

        /** Last known / stimulus world location of the target. */
        PROPERTY(Hidden)
        FVector3 Location = FVector3(0.0f);

        /** The EAISenseChannel bit(s) that triggered this event. */
        PROPERTY(Hidden)
        EAISenseChannel Sense = EAISenseChannel::Sight;

        /** Damage amount / noise loudness for hearing+damage; 0 for sight. */
        PROPERTY(Hidden)
        float Strength = 0.0f;

        // False when the target was lost, which picks OnTargetLost over OnTargetPerceived.
        PROPERTY(Hidden)
        bool bPerceived = true;
    };

    // Defined beside the perception component it delivers to, which this header cannot see.
    template<>
    struct TEventFanOut<SPerceptionEvent>
    {
        RUNTIME_API static void Deliver(ECS::FRegistry& Registry, const SPerceptionEvent& Event);
    };
}
