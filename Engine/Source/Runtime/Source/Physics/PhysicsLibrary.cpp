#include "RuntimePCH.h"

#include "PhysicsLibrary.h"

#include "Physics/PhysicsScene.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // The caller sizes the result set; past this a query reports its true total and is called again.
        constexpr size_t DefaultQueryCapacity = 256;

        Physics::IPhysicsScene* SceneOf(CWorld* World)
        {
            return World != nullptr ? World->GetPhysicsScene() : nullptr;
        }

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
        void Overlap(CWorld* World, const SQueryShape& Shape, const FVector3& Center, ECS::FEntity IgnoreEntity,
            ECollisionProfiles LayerMask, TVector<ECS::FEntity>& Out)
        {
            Physics::IPhysicsScene* Scene = SceneOf(World);
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

        bool IsFree(CWorld* World, const SQueryShape& Shape, const FVector3& Center, ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
        {
            Physics::IPhysicsScene* Scene = SceneOf(World);
            ECS::FEntity First;
            return Scene == nullptr || Scene->OverlapShape(Shape, Center, LayerMask, IgnoreOf(IgnoreEntity), TSpan<ECS::FEntity>(&First, 1)) == 0;
        }
    }

    SRayResult CPhysicsLibrary::Raycast(CWorld* World, FVector3 Start, FVector3 End, ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        const SRayResult Miss = MissOf(Start, End);
        Physics::IPhysicsScene* Scene = SceneOf(World);
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

    void CPhysicsLibrary::RaycastAll(CWorld* World, FVector3 Start, FVector3 End, ECS::FEntity IgnoreEntity,
        ECollisionProfiles LayerMask, TVector<SRayResult>& Out)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
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

    SRayResult CPhysicsLibrary::RaycastDirection(CWorld* World, FVector3 Start, FVector3 Direction, float Distance,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return Raycast(World, Start, Start + Math::Normalize(Direction) * Distance, IgnoreEntity, LayerMask);
    }

    bool CPhysicsLibrary::HasLineOfSight(CWorld* World, FVector3 From, FVector3 To, ECS::FEntity IgnoreEntity,
        ECS::FEntity TargetEntity, ECollisionProfiles LayerMask)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
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

    SRayResult CPhysicsLibrary::RaycastEntity(CWorld* World, ECS::FEntity Entity, FVector3 Start, FVector3 End)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        TOptional<SRayResult> Hit = Scene != nullptr ? Scene->CastRayAgainst(Entity, Start, End) : TOptional<SRayResult>();
        return Hit.has_value() ? Hit.value() : MissOf(Start, End);
    }

    void CPhysicsLibrary::SphereCast(CWorld* World, FVector3 Start, FVector3 End, float Radius,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        ShapeCastAll(World, SQueryShape::Sphere(Radius), Start, End, IgnoreEntity, Out, LayerMask);
    }

    SRayResult CPhysicsLibrary::SphereCastClosest(CWorld* World, FVector3 Start, FVector3 End, float Radius,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return ShapeCast(World, SQueryShape::Sphere(Radius), Start, End, IgnoreEntity, LayerMask);
    }

    SRayResult CPhysicsLibrary::BoxCast(CWorld* World, FVector3 Start, FVector3 End, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return ShapeCast(World, SQueryShape::Box(HalfExtents, Rotation), Start, End, IgnoreEntity, LayerMask);
    }

    void CPhysicsLibrary::BoxCastAll(CWorld* World, FVector3 Start, FVector3 End, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        ShapeCastAll(World, SQueryShape::Box(HalfExtents, Rotation), Start, End, IgnoreEntity, Out, LayerMask);
    }

    SRayResult CPhysicsLibrary::CapsuleCast(CWorld* World, FVector3 Start, FVector3 End, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return ShapeCast(World, SQueryShape::Capsule(Radius, HalfHeight, Rotation), Start, End, IgnoreEntity, LayerMask);
    }

    void CPhysicsLibrary::CapsuleCastAll(CWorld* World, FVector3 Start, FVector3 End, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        ShapeCastAll(World, SQueryShape::Capsule(Radius, HalfHeight, Rotation), Start, End, IgnoreEntity, Out, LayerMask);
    }

    SRayResult CPhysicsLibrary::ShapeCast(CWorld* World, SQueryShape Shape, FVector3 Start, FVector3 End,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        TOptional<SRayResult> Hit = Scene != nullptr ? Scene->CastShapeClosest(Shape, Start, End, LayerMask, IgnoreOf(IgnoreEntity)) : TOptional<SRayResult>();
        return Hit.has_value() ? Hit.value() : MissOf(Start, End);
    }

    void CPhysicsLibrary::ShapeCastAll(CWorld* World, SQueryShape Shape, FVector3 Start, FVector3 End,
        ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World))
        {
            Scene->CastShapeAll(Shape, Start, End, LayerMask, IgnoreOf(IgnoreEntity), Out);
        }
    }

    void CPhysicsLibrary::OverlapSphere(CWorld* World, FVector3 Center, float Radius,
        ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(World, SQueryShape::Sphere(Radius), Center, IgnoreEntity, LayerMask, Out);
    }

    void CPhysicsLibrary::OverlapBox(CWorld* World, FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(World, SQueryShape::Box(HalfExtents, Rotation), Center, IgnoreEntity, LayerMask, Out);
    }

    void CPhysicsLibrary::OverlapCapsule(CWorld* World, FVector3 Center, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(World, SQueryShape::Capsule(Radius, HalfHeight, Rotation), Center, IgnoreEntity, LayerMask, Out);
    }

    void CPhysicsLibrary::OverlapShape(CWorld* World, SQueryShape Shape, FVector3 Center, ECS::FEntity IgnoreEntity,
        TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(World, Shape, Center, IgnoreEntity, LayerMask, Out);
    }

    void CPhysicsLibrary::OverlapPoint(CWorld* World, FVector3 Point, ECS::FEntity IgnoreEntity,
        TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask)
    {
        Overlap(World, SQueryShape::Sphere(0.0f), Point, IgnoreEntity, LayerMask, Out);
    }

    bool CPhysicsLibrary::IsSpaceFree(CWorld* World, FVector3 Center, float Radius, ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return IsFree(World, SQueryShape::Sphere(Radius), Center, IgnoreEntity, LayerMask);
    }

    bool CPhysicsLibrary::IsBoxSpaceFree(CWorld* World, FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return IsFree(World, SQueryShape::Box(HalfExtents, Rotation), Center, IgnoreEntity, LayerMask);
    }

    bool CPhysicsLibrary::IsCapsuleSpaceFree(CWorld* World, FVector3 Center, float Radius, float HalfHeight, FQuat Rotation,
        ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask)
    {
        return IsFree(World, SQueryShape::Capsule(Radius, HalfHeight, Rotation), Center, IgnoreEntity, LayerMask);
    }

    ECS::FEntity CPhysicsLibrary::FindNearestEntity(CWorld* World, FVector3 Center, float Radius, ECS::FEntity IgnoreEntity,
        ECollisionProfiles LayerMask)
    {
        TVector<ECS::FEntity> Candidates;
        OverlapSphere(World, Center, Radius, IgnoreEntity, Candidates, LayerMask);

        ECS::FEntity Nearest = ECS::NullEntity;
        float NearestDistance = FLT_MAX;
        for (ECS::FEntity Candidate : Candidates)
        {
            const float Distance = GetDistanceToEntity(World, Candidate, Center);
            if (Distance >= 0.0f && Distance < NearestDistance)
            {
                NearestDistance = Distance;
                Nearest = Candidate;
            }
        }
        return Nearest;
    }

    FVector3 CPhysicsLibrary::GetClosestPoint(CWorld* World, ECS::FEntity Entity, FVector3 Point)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        FVector3 Closest = Point;
        if (Scene != nullptr)
        {
            Scene->GetClosestPointOnBody(Entity, Point, Closest);
        }
        return Closest;
    }

    float CPhysicsLibrary::GetDistanceToEntity(CWorld* World, ECS::FEntity Entity, FVector3 Point)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        FVector3 Closest;
        return Scene != nullptr && Scene->GetClosestPointOnBody(Entity, Point, Closest) ? Math::Length(Closest - Point) : -1.0f;
    }

    SAABB CPhysicsLibrary::GetBodyBounds(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        SAABB Bounds;
        if (Scene == nullptr || !Scene->GetBodyBounds(Entity, Bounds))
        {
            return SAABB();
        }
        return Bounds;
    }

    float CPhysicsLibrary::GetGroundHeight(CWorld* World, FVector3 Point, float SearchDistance, ECS::FEntity IgnoreEntity, float Fallback,
        ECollisionProfiles LayerMask)
    {
        const FVector3 Start(Point.x, Point.y + SearchDistance, Point.z);
        const FVector3 End(Point.x, Point.y - SearchDistance, Point.z);
        const SRayResult Hit = Raycast(World, Start, End, IgnoreEntity, LayerMask);
        return Hit.bHit ? Hit.Location.y : Fallback;
    }

    void CPhysicsLibrary::AddForce(CWorld* World, ECS::FEntity Entity, FVector3 Force)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->AddForce(Entity, Force); }
    }

    void CPhysicsLibrary::AddImpulse(CWorld* World, ECS::FEntity Entity, FVector3 Impulse)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->AddImpulse(Entity, Impulse); }
    }

    void CPhysicsLibrary::AddTorque(CWorld* World, ECS::FEntity Entity, FVector3 Torque)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->AddTorque(Entity, Torque); }
    }

    void CPhysicsLibrary::AddAngularImpulse(CWorld* World, ECS::FEntity Entity, FVector3 AngularImpulse)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->AddAngularImpulse(Entity, AngularImpulse); }
    }

    void CPhysicsLibrary::AddForceAtPosition(CWorld* World, ECS::FEntity Entity, FVector3 Force, FVector3 Position)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->AddForceAtPosition(Entity, Force, Position); }
    }

    void CPhysicsLibrary::AddImpulseAtPosition(CWorld* World, ECS::FEntity Entity, FVector3 Impulse, FVector3 Position)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->AddImpulseAtPosition(Entity, Impulse, Position); }
    }

    void CPhysicsLibrary::SetLinearVelocity(CWorld* World, ECS::FEntity Entity, FVector3 Velocity)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->SetLinearVelocity(Entity, Velocity); }
    }

    FVector3 CPhysicsLibrary::GetLinearVelocity(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetLinearVelocity(Entity) : FVector3(0.0f);
    }

    void CPhysicsLibrary::SetAngularVelocity(CWorld* World, ECS::FEntity Entity, FVector3 Velocity)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->SetAngularVelocity(Entity, Velocity); }
    }

    FVector3 CPhysicsLibrary::GetAngularVelocity(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetAngularVelocity(Entity) : FVector3(0.0f);
    }

    FVector3 CPhysicsLibrary::GetVelocityAtPoint(CWorld* World, ECS::FEntity Entity, FVector3 Point)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetVelocityAtPoint(Entity, Point) : FVector3(0.0f);
    }

    FVector3 CPhysicsLibrary::GetBodyPosition(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetBodyPosition(Entity) : FVector3(0.0f);
    }

    FQuat CPhysicsLibrary::GetBodyRotation(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetBodyRotation(Entity) : FQuat();
    }

    FVector3 CPhysicsLibrary::GetCenterOfMass(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetCenterOfMass(Entity) : FVector3(0.0f);
    }

    float CPhysicsLibrary::GetMass(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetBodyMass(Entity) : 0.0f;
    }

    void CPhysicsLibrary::SetGravityFactor(CWorld* World, ECS::FEntity Entity, float Factor)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->SetGravityFactor(Entity, Factor); }
    }

    void CPhysicsLibrary::ActivateBody(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        if (Scene == nullptr)
        {
            return;
        }
        Scene->ActivateBody(Entity);
    }

    void CPhysicsLibrary::DeactivateBody(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        if (Scene == nullptr)
        {
            return;
        }
        Scene->DeactivateBody(Entity);
    }

    bool CPhysicsLibrary::IsAwake(CWorld* World, ECS::FEntity Entity)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        if (Scene == nullptr)
        {
            return false;
        }
        return Scene->IsBodyActive(Entity);
    }

    void CPhysicsLibrary::SetSurfaceVelocity(CWorld* World, ECS::FEntity Entity, FVector3 Linear, FVector3 Angular)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->SetSurfaceVelocity(Entity, Linear, Angular); }
    }

    uint32 CPhysicsLibrary::CreateConstraint(CWorld* World, Physics::SConstraintDesc Desc)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->CreateConstraint(Desc) : 0;
    }

    uint32 CPhysicsLibrary::CreateFixedConstraint(CWorld* World, ECS::FEntity BodyA, ECS::FEntity BodyB,
        float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Fixed;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(World, Desc);
    }

    uint32 CPhysicsLibrary::CreatePointConstraint(CWorld* World, ECS::FEntity BodyA, ECS::FEntity BodyB,
        FVector3 Pivot, float BreakForce)
    {
        Physics::SConstraintDesc Desc;
        Desc.Type = EPhysicsConstraintType::Point;
        Desc.BodyA = BodyA;
        Desc.BodyB = BodyB;
        Desc.Anchor = Pivot;
        Desc.BreakForce = BreakForce;
        return CreateConstraint(World, Desc);
    }

    uint32 CPhysicsLibrary::CreateDistanceConstraint(CWorld* World, ECS::FEntity BodyA, ECS::FEntity BodyB,
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
        return CreateConstraint(World, Desc);
    }

    uint32 CPhysicsLibrary::CreateHingeConstraint(CWorld* World, ECS::FEntity BodyA, ECS::FEntity BodyB,
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
        return CreateConstraint(World, Desc);
    }

    uint32 CPhysicsLibrary::CreateSliderConstraint(CWorld* World, ECS::FEntity BodyA, ECS::FEntity BodyB,
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
        return CreateConstraint(World, Desc);
    }

    uint32 CPhysicsLibrary::CreateConeConstraint(CWorld* World, ECS::FEntity BodyA, ECS::FEntity BodyB,
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
        return CreateConstraint(World, Desc);
    }

    void CPhysicsLibrary::DestroyConstraint(CWorld* World, uint32 ConstraintID)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->DestroyConstraint(ConstraintID); }
    }

    void CPhysicsLibrary::SetConstraintEnabled(CWorld* World, uint32 ConstraintID, bool bEnabled)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->SetConstraintEnabled(ConstraintID, bEnabled); }
    }

    void CPhysicsLibrary::SetConstraintMotor(CWorld* World, uint32 ConstraintID,
        Physics::EConstraintMotorMode Mode, float Target)
    {
        if (Physics::IPhysicsScene* Scene = SceneOf(World)) { Scene->SetConstraintMotor(ConstraintID, Mode, Target); }
    }

    bool CPhysicsLibrary::IsConstraintBroken(CWorld* World, uint32 ConstraintID)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene != nullptr && Scene->IsConstraintBroken(ConstraintID);
    }

    float CPhysicsLibrary::GetConstraintValue(CWorld* World, uint32 ConstraintID)
    {
        Physics::IPhysicsScene* Scene = SceneOf(World);
        return Scene ? Scene->GetConstraintValue(ConstraintID) : 0.0f;
    }
}
