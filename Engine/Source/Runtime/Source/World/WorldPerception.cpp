#include "RuntimePCH.h"

#include "AI/Perception/PerceptionWorldState.h"
#include "Physics/PhysicsScene.h"
#include "Physics/Ray/RayCast.h"
#include "World/Entity/Components/AIStimuliSourceComponent.h"
#include "World/Entity/Components/PerceptionComponent.h"
#include "World/Entity/Components/TransformComponent.h"
#include "World/World.h"

namespace Lumina
{
    void CWorld::GetPerceivedTargets(ECS::FEntity Perceiver,
        TVector<ECS::FEntity>& Out)
    {
        const SPerceptionComponent* Perception = TryGetComponent<SPerceptionComponent>(Perceiver);
        if (Perception == nullptr)
        {
            return;
        }

        for (int32 Index = 0; Index < Perception->PerceivedCount; ++Index)
        {
            Out.push_back(Perception->PerceivedTargets[Index].Target);
        }
    }

    bool CWorld::CanSense(ECS::FEntity Perceiver, ECS::FEntity Target,
        EAISenseChannel Senses)
    {
        const SPerceptionComponent* Comp = TryGetComponent<SPerceptionComponent>(Perceiver);
        if (Comp == nullptr)
        {
            return false;
        }
        const int32 Index = Comp->Find(Target);
        return Index >= 0 && (Comp->PerceivedTargets[Index].ActiveSenses & (uint8)Senses) != 0;
    }

    SNavPoint CWorld::GetLastKnownLocation(ECS::FEntity Perceiver,
        ECS::FEntity Target)
    {
        SNavPoint Result;
        const SPerceptionComponent* Comp = TryGetComponent<SPerceptionComponent>(Perceiver);
        if (Comp == nullptr)
        {
            return Result;
        }
        const int32 Index = Comp->Find(Target);
        if (Index >= 0)
        {
            Result.bFound = true;
            Result.Point = Comp->PerceivedTargets[Index].LastKnownLocation;
        }
        return Result;
    }

    ECS::FEntity CWorld::GetClosestPerceivedTarget(ECS::FEntity Perceiver)
    {
        const SPerceptionComponent* Comp = TryGetComponent<SPerceptionComponent>(Perceiver);
        return Comp != nullptr ? Comp->GetClosestPerceivedTarget() : ECS::NullEntity;
    }

    bool CWorld::HasPerceptionLineOfSight(ECS::FEntity From, ECS::FEntity To)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        ECS::FRegistry& Registry = EntityRegistry;
        const STransformComponent* FromXf = Registry.TryGet<STransformComponent>(From);
        const STransformComponent* ToXf = Registry.TryGet<STransformComponent>(To);
        if (Scene == nullptr || FromXf == nullptr || ToXf == nullptr)
        {
            return false;
        }

        FVector3 Start = FromXf->GetWorldLocation();
        FVector3 End = ToXf->GetWorldLocation();
        if (const SPerceptionComponent* Comp = Registry.TryGet<SPerceptionComponent>(From))
        {
            Start = Start + Comp->EyeOffset;
        }
        if (const SAIStimuliSourceComponent* Source = Registry.TryGet<SAIStimuliSourceComponent>(To))
        {
            End = End + Source->SightTargetOffset;
        }

        SRayCastSettings Ray;
        Ray.Start = Start;
        Ray.End = End;
        Ray.LayerMask = ECollisionProfiles::Static | ECollisionProfiles::Dynamic;
        const ECS::FEntity FromBody = From;
        if (FromBody != ECS::NullEntity)
        {
            Ray.IgnoreEntities.push_back(FromBody);
        }

        const TOptional<SRayResult> Hit = Scene->CastRay(Ray);
        return !Hit.has_value() || Hit->Entity == To.Value;
    }

    void CWorld::ReportPerceptionNoise(FVector3 Location, float Loudness,
        ECS::FEntity Instigator)
    {
        FAIStimulusEvent Event;
        Event.Sense = EAISenseChannel::Hearing;
        Event.Instigator = Instigator;
        Event.Target = ECS::NullEntity;
        Event.Location = Location;
        Event.Strength = Loudness;
        Perception::EnqueueStimulus(Perception::GetOrCreateState(EntityRegistry), Event);
    }

    void CWorld::ReportPerceptionDamage(ECS::FEntity Victim, ECS::FEntity Instigator,
        FVector3 HitLocation, float Amount)
    {
        FAIStimulusEvent Event;
        Event.Sense = EAISenseChannel::Damage;
        Event.Instigator = Instigator;
        Event.Target = Victim;
        Event.Location = HitLocation;
        Event.Strength = Amount;
        Perception::EnqueueStimulus(Perception::GetOrCreateState(EntityRegistry), Event);
    }

    void CWorld::RegisterPerceptionSource(ECS::FEntity Entity,
        const FName& AffiliationTag, EAISenseChannel Senses)
    {
        SAIStimuliSourceComponent& Source = GetOrEmplaceComponent<SAIStimuliSourceComponent>(Entity);
        if ((uint8)Senses != 0)
        {
            Source.RegisteredSenses = Senses;
        }
        if (!AffiliationTag.IsNone())
        {
            SGameplayTag Tag;
            Tag.TagName = AffiliationTag;
            Source.AffiliationTags.AddTag(Tag);
        }
    }

    void CWorld::AddDetectableTag(ECS::FEntity Perceiver, const FName& Tag)
    {
        if (Tag.IsNone())
        {
            return;
        }
        SGameplayTag Detectable;
        Detectable.TagName = Tag;
        GetOrEmplaceComponent<SPerceptionComponent>(Perceiver).DetectableTags.AddTag(Detectable);
    }
}
