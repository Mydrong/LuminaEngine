#pragma once

#include "AI/Navigation/NavTypes.h"
#include "AI/Perception/PerceptionTypes.h"
#include "Containers/Vector.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "GameplayTags/GameplayTag.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "World/ECS/Entity.h"
#include "PerceptionLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    /** Reads of a perceiver's tracked targets, which belong to the perception component. */
    REFLECT()
    class RUNTIME_API CPerceptionLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** The targets the perceiver currently senses, empty when it has no perception component. */
        FUNCTION()
        static void GetPerceivedTargets(CWorld* World, ECS::FEntity Perceiver, TVector<ECS::FEntity>& Out);

        /** Whether any of the named sense channels currently detects the target. */
        FUNCTION()
        static bool CanSense(CWorld* World, ECS::FEntity Perceiver, ECS::FEntity Target, EAISenseChannel Senses);

        /** Held through the forget window, and not found once the target drops out of memory. */
        FUNCTION()
        static SNavPoint GetLastKnownLocation(CWorld* World, ECS::FEntity Perceiver, ECS::FEntity Target);

        /** Nearest perceived target to the perceiver's eye, or the null entity. */
        FUNCTION()
        static ECS::FEntity GetClosestPerceivedTarget(CWorld* World, ECS::FEntity Perceiver);

        /** One shot line of sight test, applying the eye and aim offsets when those components exist. */
        FUNCTION()
        static bool HasLineOfSight(CWorld* World, ECS::FEntity From, ECS::FEntity To);

        /** Heard by every perceiver in range that detects the instigator's affiliation. */
        FUNCTION()
        static void ReportNoise(CWorld* World, FVector3 Location, float Loudness, ECS::FEntity Instigator);

        /** The victim immediately perceives its attacker, whatever their affiliation. */
        FUNCTION()
        static void ReportDamage(CWorld* World, ECS::FEntity Victim, ECS::FEntity Instigator,
            FVector3 HitLocation, float Amount);

        /** Makes the entity sensible. Richer affiliation sets are authored in the editor. */
        FUNCTION()
        static void RegisterSource(CWorld* World, ECS::FEntity Entity, const FName& AffiliationTag,
            EAISenseChannel Senses);

        /** Only sources carrying a matching tag are sensed, and an empty filter senses everyone. */
        FUNCTION()
        static void AddDetectableTag(CWorld* World, ECS::FEntity Perceiver, const FName& Tag);
    };
}
