#include "RuntimePCH.h"

#include "World/World.h"

#include "Physics/PhysicsScene.h"
#include "World/Entity/Components/CharacterComponent.h"
#include "World/Entity/Components/PhysicsComponent.h"

namespace Lumina
{
    namespace
    {
        // The caller sizes the result set; past this a query reports its true total and is called again.
        constexpr size_t DefaultQueryCapacity = 256;

        void StageIgnore(ECS::FEntity Entity, TFixedVector<ECS::FEntity, MaxInlineIgnoreEntities>& Out)
        {
            if (Entity != ECS::NullEntity)
            {
                Out.push_back(Entity);
            }
        }

        using FIgnoreList = TFixedVector<ECS::FEntity, MaxInlineIgnoreEntities>;

        FIgnoreList IgnoreOf(ECS::FEntity Entity)
        {
            FIgnoreList Ignore;
            StageIgnore(Entity, Ignore);
            return Ignore;
        }

        SRayResult MissOf(const FVector3& Start, const FVector3& End)
        {
            SRayResult Miss;
            Miss.Start = Start;
            Miss.End = End;
            return Miss;
        }

        // The scene writes into a span, so the result vector is sized first and trimmed to what it wrote.
        void Overlap(Physics::IPhysicsScene* Scene, const SQueryShape& Shape, const FVector3& Center, ECS::FEntity IgnoreEntity,
            ECollisionProfiles LayerMask, TVector<ECS::FEntity>& Out)
        {
            if (Scene == nullptr)
            {
                return;
            }

            const size_t Start = Out.size();
            Out.resize(Start + DefaultQueryCapacity);
            const int32 Written = Scene->OverlapShape(Shape, Center, LayerMask, IgnoreOf(IgnoreEntity),
                TSpan<ECS::FEntity>(Out.data() + Start, DefaultQueryCapacity));
            Out.resize(Start + (size_t)(Written > 0 ? Written : 0));
        }

        bool IsFree(Physics::IPhysicsScene* Scene, const SQueryShape& Shape, const FVector3& Center, ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
        {
            ECS::FEntity First;
            return Scene == nullptr || Scene->OverlapShape(Shape, Center, LayerMask, IgnoreOf(IgnoreEntity), TSpan<ECS::FEntity>(&First, 1)) == 0;
        }
    }

    SRayResult CWorld::Raycast(FVector3 Start, FVector3 End, ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        const SRayResult Miss = MissOf(Start, End);
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        if (Scene == nullptr)
        {
            return Miss;
        }

        SRayCastSettings Settings;
        Settings.Start = Start;
        Settings.End = End;
        Settings.LayerMask = LayerMask;
        StageIgnore(IgnoreEntity, Settings.IgnoreEntities);

        TOptional<SRayResult> Result = Scene->CastRay(Settings);
        return Result.has_value() ? Result.value() : Miss;
    }

    void CWorld::RaycastAll(FVector3 Start, FVector3 End, ECS::FEntity IgnoreEntity,
        ECollisionProfiles LayerMask, TVector<SRayResult>& Out)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        if (Scene == nullptr)
        {
            return;
        }

        SRayCastSettings Settings;
        Settings.Start = Start;
        Settings.End = End;
        Settings.LayerMask = LayerMask;
        StageIgnore(IgnoreEntity, Settings.IgnoreEntities);

        Scene->CastRayAll(Settings, Out);
    }

    SRayResult CWorld::RaycastDirection(FVector3 Start, FVector3 Direction, float Distance,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return Raycast(Start, Start + Math::Normalize(Direction) * Distance, IgnoreEntity, LayerMask);
    }

    bool CWorld::HasLineOfSight(FVector3 From, FVector3 To, ECS::FEntity IgnoreEntity,
        ECS::FEntity TargetEntity, ECollisionProfiles LayerMask)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        if (Scene == nullptr)
        {
            return true;
        }

        SRayCastSettings Settings;
        Settings.Start = From;
        Settings.End = To;
        Settings.LayerMask = LayerMask;
        StageIgnore(IgnoreEntity, Settings.IgnoreEntities);
        StageIgnore(TargetEntity, Settings.IgnoreEntities);
        return !Scene->CastRayAny(Settings);
    }

    SRayResult CWorld::RaycastEntity(ECS::FEntity Entity, FVector3 Start, FVector3 End)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        TOptional<SRayResult> Hit = Scene != nullptr ? Scene->CastRayAgainst(Entity, Start, End) : TOptional<SRayResult>();
        return Hit.has_value() ? Hit.value() : MissOf(Start, End);
    }

    void CWorld::SphereCast(FVector3 Start, FVector3 End, float Radius,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        ShapeCastAll(SQueryShape::Sphere(Radius), Start, End, IgnoreEntity, Out, LayerMask);
    }

    SRayResult CWorld::SphereCastClosest(FVector3 Start, FVector3 End, float Radius,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return ShapeCast(SQueryShape::Sphere(Radius), Start, End, IgnoreEntity, LayerMask);
    }

    SRayResult CWorld::BoxCast(FVector3 Start, FVector3 End, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return ShapeCast(SQueryShape::Box(HalfExtents, Rotation), Start, End, IgnoreEntity, LayerMask);
    }

    void CWorld::BoxCastAll(FVector3 Start, FVector3 End, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        ShapeCastAll(SQueryShape::Box(HalfExtents, Rotation), Start, End, IgnoreEntity, Out, LayerMask);
    }

    SRayResult CWorld::CapsuleCast(FVector3 Start, FVector3 End, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return ShapeCast(SQueryShape::Capsule(Radius, HalfHeight, Rotation), Start, End, IgnoreEntity, LayerMask);
    }

    void CWorld::CapsuleCastAll(FVector3 Start, FVector3 End, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        ShapeCastAll(SQueryShape::Capsule(Radius, HalfHeight, Rotation), Start, End, IgnoreEntity, Out, LayerMask);
    }

    SRayResult CWorld::ShapeCast(SQueryShape Shape, FVector3 Start, FVector3 End,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        TOptional<SRayResult> Hit = Scene != nullptr ? Scene->CastShapeClosest(Shape, Start, End, LayerMask, IgnoreOf(IgnoreEntity)) : TOptional<SRayResult>();
        return Hit.has_value() ? Hit.value() : MissOf(Start, End);
    }

    void CWorld::ShapeCastAll(SQueryShape Shape, FVector3 Start, FVector3 End,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene())
        {
            Scene->CastShapeAll(Shape, Start, End, LayerMask, IgnoreOf(IgnoreEntity), Out);
        }
    }

    void CWorld::OverlapSphere(FVector3 Center, float Radius,
        ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(GetPhysicsScene(), SQueryShape::Sphere(Radius), Center, IgnoreEntity, LayerMask, Out);
    }

    void CWorld::OverlapBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(GetPhysicsScene(), SQueryShape::Box(HalfExtents, Rotation), Center, IgnoreEntity, LayerMask, Out);
    }

    void CWorld::OverlapCapsule(FVector3 Center, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(GetPhysicsScene(), SQueryShape::Capsule(Radius, HalfHeight, Rotation), Center, IgnoreEntity, LayerMask, Out);
    }

    void CWorld::OverlapShape(SQueryShape Shape, FVector3 Center, ECS::FEntity IgnoreEntity,
        TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(GetPhysicsScene(), Shape, Center, IgnoreEntity, LayerMask, Out);
    }

    void CWorld::OverlapPoint(FVector3 Point, ECS::FEntity IgnoreEntity,
        TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(GetPhysicsScene(), SQueryShape::Sphere(0.0f), Point, IgnoreEntity, LayerMask, Out);
    }

    bool CWorld::IsSpaceFree(FVector3 Center, float Radius, ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return IsFree(GetPhysicsScene(), SQueryShape::Sphere(Radius), Center, IgnoreEntity, LayerMask);
    }

    bool CWorld::IsBoxSpaceFree(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return IsFree(GetPhysicsScene(), SQueryShape::Box(HalfExtents, Rotation), Center, IgnoreEntity, LayerMask);
    }

    bool CWorld::IsCapsuleSpaceFree(FVector3 Center, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return IsFree(GetPhysicsScene(), SQueryShape::Capsule(Radius, HalfHeight, Rotation), Center, IgnoreEntity, LayerMask);
    }

    ECS::FEntity CWorld::FindNearestEntity(FVector3 Center, float Radius, ECS::FEntity IgnoreEntity,
        ECollisionProfiles LayerMask)
    {
        TVector<ECS::FEntity> Candidates;
        OverlapSphere(Center, Radius, IgnoreEntity, Candidates, LayerMask);

        ECS::FEntity Nearest = ECS::NullEntity;
        float NearestDistance = FLT_MAX;
        for (ECS::FEntity Candidate : Candidates)
        {
            const float Distance = GetDistanceToEntity(Candidate, Center);
            if (Distance >= 0.0f && Distance < NearestDistance)
            {
                NearestDistance = Distance;
                Nearest = Candidate;
            }
        }
        return Nearest;
    }

    FVector3 CWorld::GetClosestPoint(ECS::FEntity Entity, FVector3 Point)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        FVector3 Closest = Point;
        if (Scene != nullptr)
        {
            Scene->GetClosestPointOnBody(Entity, Point, Closest);
        }
        return Closest;
    }

    float CWorld::GetDistanceToEntity(ECS::FEntity Entity, FVector3 Point)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        FVector3 Closest;
        return Scene != nullptr && Scene->GetClosestPointOnBody(Entity, Point, Closest) ? Math::Length(Closest - Point) : -1.0f;
    }

    SAABB CWorld::GetBodyBounds(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        SAABB Bounds;
        if (Scene == nullptr || !Scene->GetBodyBounds(Entity, Bounds))
        {
            return SAABB();
        }
        return Bounds;
    }

    void CWorld::RebuildPhysics(ECS::FEntity Entity)
    {
        if (!IsValidEntity(Entity))
        {
            return;
        }

        // The patch is what physics listens for, so the scene re-reads every field on its next step.
        ECS::FRegistry& Registry = EntityRegistry;
        if (Registry.HasAll<SRigidBodyComponent>(Entity))
        {
            Registry.Patch<SRigidBodyComponent>(Entity);
        }
        if (Registry.HasAll<SCharacterPhysicsComponent>(Entity))
        {
            Registry.Patch<SCharacterPhysicsComponent>(Entity);
        }
    }

    void CWorld::SetCapsuleSize(ECS::FEntity Entity, float Radius, float HalfHeight)
    {
        if (!IsValidEntity(Entity))
        {
            return;
        }

        ECS::FRegistry& Registry = EntityRegistry;
        if (SCharacterPhysicsComponent* Character = Registry.TryGet<SCharacterPhysicsComponent>(Entity))
        {
            Character->Radius     = Math::Max(Radius, 0.0f);
            Character->HalfHeight = Math::Max(HalfHeight, 0.0f);
        }
        else if (SCapsuleColliderComponent* Capsule = Registry.TryGet<SCapsuleColliderComponent>(Entity))
        {
            Capsule->Radius     = Math::Max(Radius, 0.0f);
            Capsule->HalfHeight = Math::Max(HalfHeight, 0.0f);
        }
        RebuildPhysics(Entity);
    }

    void CWorld::SetBoxHalfExtent(ECS::FEntity Entity, FVector3 HalfExtent)
    {
        if (!IsValidEntity(Entity))
        {
            return;
        }

        if (SBoxColliderComponent* Box = EntityRegistry.TryGet<SBoxColliderComponent>(Entity))
        {
            Box->HalfExtent = Math::Max(HalfExtent, FVector3(0.0f));
            RebuildPhysics(Entity);
        }
    }

    void CWorld::SetSphereRadius(ECS::FEntity Entity, float Radius)
    {
        if (!IsValidEntity(Entity))
        {
            return;
        }

        if (SSphereColliderComponent* Sphere = EntityRegistry.TryGet<SSphereColliderComponent>(Entity))
        {
            Sphere->Radius = Math::Max(Radius, 0.0f);
            RebuildPhysics(Entity);
        }
    }

    void CWorld::SetCollisionProfile(ECS::FEntity Entity, SCollisionProfile Profile)
    {
        if (!IsValidEntity(Entity))
        {
            return;
        }

        ECS::FRegistry& Registry = EntityRegistry;
        if (SRigidBodyComponent* Body = Registry.TryGet<SRigidBodyComponent>(Entity))
        {
            Body->CollisionProfile = Profile;
        }
        if (SCharacterPhysicsComponent* Character = Registry.TryGet<SCharacterPhysicsComponent>(Entity))
        {
            Character->CollisionProfile = Profile;
        }
        RebuildPhysics(Entity);
    }

    float CWorld::GetGroundHeight(FVector3 Point, float SearchDistance, ECS::FEntity IgnoreEntity, float Fallback,
        ECollisionProfiles LayerMask)
    {
        const FVector3 Start(Point.x, Point.y + SearchDistance, Point.z);
        const FVector3 End(Point.x, Point.y - SearchDistance, Point.z);
        const SRayResult Hit = Raycast(Start, End, IgnoreEntity, LayerMask);
        return Hit.bHit ? Hit.Location.y : Fallback;
    }

    void CWorld::AddForce(ECS::FEntity Entity, FVector3 Force)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddForce(Entity, Force); }
    }

    void CWorld::AddImpulse(ECS::FEntity Entity, FVector3 Impulse)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddImpulse(Entity, Impulse); }
    }

    void CWorld::AddTorque(ECS::FEntity Entity, FVector3 Torque)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddTorque(Entity, Torque); }
    }

    void CWorld::AddAngularImpulse(ECS::FEntity Entity, FVector3 AngularImpulse)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddAngularImpulse(Entity, AngularImpulse); }
    }

    void CWorld::AddForceAtPosition(ECS::FEntity Entity, FVector3 Force, FVector3 Position)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddForceAtPosition(Entity, Force, Position); }
    }

    void CWorld::AddImpulseAtPosition(ECS::FEntity Entity, FVector3 Impulse, FVector3 Position)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddImpulseAtPosition(Entity, Impulse, Position); }
    }

    void CWorld::SetLinearVelocity(ECS::FEntity Entity, FVector3 Velocity)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetLinearVelocity(Entity, Velocity); }
    }

    FVector3 CWorld::GetLinearVelocity(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetLinearVelocity(Entity) : FVector3(0.0f);
    }

    void CWorld::SetAngularVelocity(ECS::FEntity Entity, FVector3 Velocity)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetAngularVelocity(Entity, Velocity); }
    }

    FVector3 CWorld::GetAngularVelocity(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetAngularVelocity(Entity) : FVector3(0.0f);
    }

    SPhysicsBodyTarget CWorld::GetRagdollBody(ECS::FEntity Entity, FName Bone)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetRagdollBoneTarget(Entity, Bone) : SPhysicsBodyTarget();
    }

    void CWorld::AddImpulseToBody(SPhysicsBodyTarget Body, FVector3 Impulse, FVector3 Position)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->AddImpulseAtTarget(Body, Impulse, Position); }
    }

    void CWorld::SetBodyLinearVelocity(SPhysicsBodyTarget Body, FVector3 Velocity)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetTargetLinearVelocity(Body, Velocity); }
    }

    void CWorld::SetBodyAngularVelocity(SPhysicsBodyTarget Body, FVector3 Velocity)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetTargetAngularVelocity(Body, Velocity); }
    }

    FVector3 CWorld::GetBodyLinearVelocity(SPhysicsBodyTarget Body)
    {
        Physics::FPhysicsBodyState State;
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene && Scene->TryGetTargetState(Body, State) ? State.LinearVelocity : FVector3(0.0f);
    }

    FVector3 CWorld::GetVelocityAtPoint(ECS::FEntity Entity, FVector3 Point)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetVelocityAtPoint(Entity, Point) : FVector3(0.0f);
    }

    FVector3 CWorld::GetBodyPosition(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetBodyPosition(Entity) : FVector3(0.0f);
    }

    FQuat CWorld::GetBodyRotation(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetBodyRotation(Entity) : FQuat();
    }

    FVector3 CWorld::GetCenterOfMass(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetCenterOfMass(Entity) : FVector3(0.0f);
    }

    float CWorld::GetMass(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetBodyMass(Entity) : 0.0f;
    }

    void CWorld::SetGravityFactor(ECS::FEntity Entity, float Factor)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetGravityFactor(Entity, Factor); }
    }

    void CWorld::ActivateBody(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        if (Scene == nullptr)
        {
            return;
        }
        Scene->ActivateBody(Entity);
    }

    void CWorld::DeactivateBody(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        if (Scene == nullptr)
        {
            return;
        }
        Scene->DeactivateBody(Entity);
    }

    bool CWorld::IsAwake(ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        if (Scene == nullptr)
        {
            return false;
        }
        return Scene->IsBodyActive(Entity);
    }

    void CWorld::SetSurfaceVelocity(ECS::FEntity Entity, FVector3 Linear, FVector3 Angular)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetSurfaceVelocity(Entity, Linear, Angular); }
    }

    uint32 CWorld::CreateConstraint(Physics::SConstraintDesc Desc)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->CreateConstraint(Desc) : 0;
    }

    uint32 CWorld::CreateFixedConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
        float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Fixed;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(Desc);
    }

    uint32 CWorld::CreatePointConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
        FVector3 Pivot, float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Point;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.Anchor = Pivot;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(Desc);
    }

    uint32 CWorld::CreateDistanceConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
        FVector3 PointA, FVector3 PointB, float MinDistance, float MaxDistance, float Frequency,
        float Damping, float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Distance;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.Anchor = PointA;
        Desc.AnchorB = PointB;
        Desc.MinLimit = MinDistance;
        Desc.MaxLimit = MaxDistance;
        Desc.bHasLimits = MinDistance >= 0.0f || MaxDistance >= 0.0f;
        Desc.LimitFrequency = Frequency;
        Desc.LimitDamping = Damping;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(Desc);
    }

    uint32 CWorld::CreateHingeConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
        FVector3 Pivot, FVector3 Axis, float MinAngle, float MaxAngle, bool bLimited, float MaxFrictionTorque,
        float MotorTorqueLimit, float MotorFrequency, float MotorDamping, float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Hinge;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.Anchor = Pivot;
        Desc.Axis = Axis;
        Desc.MinLimit = MinAngle;
        Desc.MaxLimit = MaxAngle;
        Desc.bHasLimits = bLimited;
        Desc.MaxFriction = MaxFrictionTorque;
        Desc.MotorTorqueLimit = MotorTorqueLimit;
        Desc.MotorFrequency = MotorFrequency;
        Desc.MotorDamping = MotorDamping;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(Desc);
    }

    uint32 CWorld::CreateSliderConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
        FVector3 Pivot, FVector3 Axis, float MinDistance, float MaxDistance, bool bLimited,
        float MaxFrictionForce, float MotorForceLimit, float MotorFrequency, float MotorDamping,
        float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Slider;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.Anchor = Pivot;
        Desc.Axis = Axis;
        Desc.MinLimit = MinDistance;
        Desc.MaxLimit = MaxDistance;
        Desc.bHasLimits = bLimited;
        Desc.MaxFriction = MaxFrictionForce;
        Desc.MotorForceLimit = MotorForceLimit;
        Desc.MotorFrequency = MotorFrequency;
        Desc.MotorDamping = MotorDamping;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(Desc);
    }

    uint32 CWorld::CreateConeConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
        FVector3 Pivot, FVector3 TwistAxis, float HalfConeAngle, float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Cone;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.Anchor = Pivot;
        Desc.Axis = TwistAxis;
        Desc.HalfConeAngle = HalfConeAngle;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(Desc);
    }

    void CWorld::DestroyConstraint(uint32 ConstraintID)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->DestroyConstraint(ConstraintID); }
    }

    void CWorld::SetConstraintEnabled(uint32 ConstraintID, bool bEnabled)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetConstraintEnabled(ConstraintID, bEnabled); }
    }

    void CWorld::SetConstraintMotor(uint32 ConstraintID,
        Physics::EConstraintMotorMode Mode, float Target)
    {
        if (Physics::IPhysicsScene* Scene = GetPhysicsScene()) { Scene->SetConstraintMotor(ConstraintID, Mode, Target); }
    }

    bool CWorld::IsConstraintBroken(uint32 ConstraintID)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene != nullptr && Scene->IsConstraintBroken(ConstraintID);
    }

    float CWorld::GetConstraintValue(uint32 ConstraintID)
    {
        Physics::IPhysicsScene* Scene = GetPhysicsScene();
        return Scene ? Scene->GetConstraintValue(ConstraintID) : 0.0f;
    }
}
