#include <gtest/gtest.h>
#include "Core/Object/ObjectCore.h"
#include "Physics/API/Box3D/Box3DPhysicsScene.h"
#include "Physics/API/Box3D/Box3DRagdollHandle.h"
#include "World/World.h"
#include "World/ECS/EventDispatcher.h"
#include "World/Entity/Components/PhysicsComponent.h"
#include "World/Entity/Components/TransformComponent.h"
#include "World/Subsystems/WorldSettings.h"

using namespace Lumina;

namespace
{
    class PhysicsSceneLifecycle : public testing::Test
    {
    protected:
        void SetUp() override
        {
            RegisterPendingTypes();
            World = NewObject<CWorld>(nullptr, NAME_None, FGuid::New(), OF_Transient);
            Registry = &ECS::GetWorldRegistry(*World);
            Registry->Ctx().Emplace<ECS::FEventDispatcher*>(&Dispatcher);
            Scene = MakeUnique<Physics::FBox3DPhysicsScene>(World);
            Scene->Simulate();
            Dt = 1.0f / Math::Max(10.0f, World->GetDefaultWorldSettings().PhysicsHz);
        }

        void TearDown() override
        {
            Scene->StopSimulate();
            Scene.reset();
            Registry->Ctx().Erase<ECS::FEventDispatcher*>();
        }

        ECS::FEntity Spawn(float X = 0.0f, bool bCollider = true)
        {
            const ECS::FEntity Entity = Registry->Create();
            Registry->Emplace<STransformComponent>(Entity).SetLocation(FVector3(X, 0.0f, 0.0f));
            auto& Body = Registry->Emplace<SRigidBodyComponent>(Entity);
            Body.bUseGravity = false;
            Body.bOverrideMass = true;
            Body.Mass = 1.0f;
            Body.LinearDamping = 0.0f;
            Body.AngularDamping = 0.0f;
            if (bCollider) { Registry->Emplace<SSphereColliderComponent>(Entity).Radius = 0.5f; }
            return Entity;
        }

        SRayCastSettings Ray(float X = 0.0f)
        {
            SRayCastSettings Query;
            Query.Start = FVector3(X, 0.0f, -3.0f);
            Query.End = FVector3(X, 0.0f, 3.0f);
            return Query;
        }

        CWorld* World = nullptr;
        ECS::FRegistry* Registry = nullptr;
        ECS::FEventDispatcher Dispatcher;
        TUniquePtr<Physics::FBox3DPhysicsScene> Scene;
        float Dt = 0.0f;
    };
}

TEST_F(PhysicsSceneLifecycle, ReadsNeverCreatePendingBodies)
{
    const ECS::FEntity Entity = Spawn();
    Physics::FPhysicsBodyState State;
    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_FALSE(Scene->TryGetBodyState(Entity, State));
        EXPECT_FALSE(Scene->CastRay(Ray()).has_value());
        EXPECT_EQ(Scene->GetBodyCount(), 0u);
    }
    EXPECT_EQ(Scene->GetBodyStatus(Entity), Physics::EPhysicsBodyStatus::Pending);
    Scene->Update(0.0);
    EXPECT_TRUE(Scene->TryGetBodyState(Entity, State));
    EXPECT_EQ(Scene->GetBodyCount(), 1u);
}

TEST_F(PhysicsSceneLifecycle, SpawnCommandsPreserveOrderAcrossAFrameWithoutAStep)
{
    const ECS::FEntity Entity = Spawn();
    Scene->SetLinearVelocity(Entity, FVector3(2.0f, 0.0f, 0.0f));
    Scene->AddImpulse(Entity, FVector3(3.0f, 0.0f, 0.0f));
    Scene->Update(0.0);
    EXPECT_FLOAT_EQ(Scene->GetLinearVelocity(Entity).x, 0.0f);
    Scene->Update(Dt);
    EXPECT_NEAR(Scene->GetLinearVelocity(Entity).x, 5.0f, 0.001f);
    Scene->SetLinearVelocity(Entity, FVector3(7.0f, 0.0f, 0.0f));
    Scene->AddImpulse(Entity, FVector3(3.0f, 0.0f, 0.0f));
    Scene->SetLinearVelocity(Entity, FVector3(11.0f, 0.0f, 0.0f));
    Scene->Update(Dt);
    EXPECT_NEAR(Scene->GetLinearVelocity(Entity).x, 11.0f, 0.001f);
}

TEST_F(PhysicsSceneLifecycle, FixedCallbackCanSpawnAndLaunchInTheSameStep)
{
    ECS::FEntity Entity;
    Scene->SetPreStepCallback([&](float)
    {
        Entity = Spawn();
        Scene->SetLinearVelocity(Entity, FVector3(4.0f, 0.0f, 0.0f));
    });
    Scene->Update(Dt);
    EXPECT_NEAR(Scene->GetLinearVelocity(Entity).x, 4.0f, 0.001f);
    EXPECT_GT(Scene->GetBodyPosition(Entity).x, 0.0f);
}

TEST_F(PhysicsSceneLifecycle, CommandsWaitForALateCollider)
{
    const ECS::FEntity Entity = Spawn(0.0f, false);
    Scene->AddImpulse(Entity, FVector3(6.0f, 0.0f, 0.0f));
    Scene->Update(Dt);
    EXPECT_EQ(Scene->GetBodyCount(), 0u);
    Registry->Emplace<SSphereColliderComponent>(Entity).Radius = 0.5f;
    Scene->Update(Dt);
    EXPECT_NEAR(Scene->GetLinearVelocity(Entity).x, 6.0f, 0.001f);
}

TEST_F(PhysicsSceneLifecycle, RemovedComponentsCancelTheirPendingCommands)
{
    const ECS::FEntity Entity = Spawn();
    Scene->AddImpulse(Entity, FVector3(9.0f, 0.0f, 0.0f));
    Registry->Remove<SRigidBodyComponent>(Entity);
    Registry->Emplace<SRigidBodyComponent>(Entity).bUseGravity = false;
    Scene->Update(Dt);
    EXPECT_EQ(Scene->GetBodyCount(), 1u);
    EXPECT_FLOAT_EQ(Scene->GetLinearVelocity(Entity).x, 0.0f);
}

TEST_F(PhysicsSceneLifecycle, DestroyedEntitiesCancelCreationAndCommands)
{
    const ECS::FEntity Entity = Spawn();
    Scene->AddImpulse(Entity, FVector3(9.0f, 0.0f, 0.0f));
    Registry->Destroy(Entity);
    const ECS::FEntity Replacement = Spawn();
    Scene->Update(Dt);
    EXPECT_EQ(Scene->GetBodyCount(), 1u);
    EXPECT_FLOAT_EQ(Scene->GetLinearVelocity(Replacement).x, 0.0f);
    EXPECT_EQ(Scene->GetBodyStatus(Entity), Physics::EPhysicsBodyStatus::Missing);
}

TEST_F(PhysicsSceneLifecycle, RetainedHitTargetsCannotReachRecycledBodies)
{
    const ECS::FEntity Entity = Spawn();
    Scene->Update(0.0);
    const auto Hit = Scene->CastRay(Ray());
    ASSERT_TRUE(Hit.has_value());
    Registry->Destroy(Entity);
    const ECS::FEntity Replacement = Spawn();
    Scene->Update(0.0);
    Physics::FPhysicsBodyState State;
    EXPECT_FALSE(Scene->TryGetTargetState(Hit->Target, State));
    Scene->AddForceAtTarget(Hit->Target, FVector3(1000.0f, 0.0f, 0.0f), FVector3(0.0f));
    Scene->Update(Dt);
    EXPECT_FLOAT_EQ(Scene->GetLinearVelocity(Replacement).x, 0.0f);
}

TEST_F(PhysicsSceneLifecycle, EntityFiltersNeedNoBodyLookup)
{
    const ECS::FEntity Entity = Spawn();
    auto Query = Ray();
    Query.IgnoreEntities.push_back(Entity);
    Scene->Update(0.0);
    EXPECT_TRUE(Scene->CastRay(Ray()).has_value());
    EXPECT_FALSE(Scene->CastRay(Query).has_value());
}

TEST_F(PhysicsSceneLifecycle, ForceIsConsumedByOneFixedStep)
{
    const ECS::FEntity Entity = Spawn();
    Scene->AddForce(Entity, FVector3(12.0f, 0.0f, 0.0f));
    Scene->Update(0.0);
    Scene->Update(2.0f * Dt);
    EXPECT_NEAR(Scene->GetLinearVelocity(Entity).x, 12.0f * Dt, 0.001f);
}

TEST_F(PhysicsSceneLifecycle, RepeatedColliderChangesDoNotDuplicateBodies)
{
    const ECS::FEntity Entity = Spawn();
    Registry->Patch<SSphereColliderComponent>(Entity, [](auto& Sphere) { Sphere.Radius = 0.75f; });
    Registry->Patch<SRigidBodyComponent>(Entity, [](auto& Body) { Body.Mass = 2.0f; });
    Scene->Update(0.0);
    EXPECT_EQ(Scene->GetBodyCount(), 1u);
    EXPECT_NEAR(Scene->GetBodyMass(Entity), 2.0f, 0.001f);
}

TEST_F(PhysicsSceneLifecycle, ConstraintsWaitForBothPendingBodies)
{
    const ECS::FEntity A = Spawn(-2.0f);
    const ECS::FEntity B = Spawn(2.0f, false);
    Physics::SConstraintDesc Desc;
    Desc.BodyA = A;
    Desc.BodyB = B;
    const uint32 Constraint = Scene->CreateConstraint(Desc);
    ASSERT_NE(Constraint, 0u);
    Scene->Update(0.0);
    EXPECT_EQ((int)Scene->GetLiveJointCount(), 0);
    Registry->Emplace<SSphereColliderComponent>(B);
    Scene->Update(0.0);
    EXPECT_EQ((int)Scene->GetLiveJointCount(), 1);
    Scene->DestroyConstraint(Constraint);
    Scene->Update(0.0);
    EXPECT_EQ((int)Scene->GetLiveJointCount(), 0);
}

TEST_F(PhysicsSceneLifecycle, RagdollRequestsOwnTemporaryPoseDataAndFilterByOwner)
{
    const ECS::FEntity Owner = Registry->Create();
    TSharedPtr<FPhysicsRagdollHandle> Handle;
    {
        FSkeletonResource Skeleton;
        FSkeletonResource::FBoneInfo Bone;
        Bone.Name = FName("root");
        Bone.ParentIndex = Constants::kIndexNone;
        Bone.LocalTransform = FMatrix4(1.0f);
        Bone.InvBindMatrix = FMatrix4(1.0f);
        Skeleton.Bones.push_back(Bone);
        Skeleton.BoneNameToIndex[Bone.Name] = 0;
        TVector<FMatrix4> Globals{ FMatrix4(1.0f) };
        Physics::FRagdollDesc Desc;
        Desc.Entity = Owner;
        Desc.Skeleton = &Skeleton;
        Desc.ComponentBoneGlobals = &Globals;
        Desc.EntityToWorld = FMatrix4(1.0f);
        Handle = Scene->CreateRagdoll(Desc);
    }
    ASSERT_TRUE(Handle);
    EXPECT_FALSE(Scene->IsRagdollReady(*Handle));
    Scene->Update(0.0);
    EXPECT_TRUE(Scene->IsRagdollReady(*Handle));
    EXPECT_EQ(Scene->GetBodyCount(), 1u);
    auto Query = Ray();
    Query.IgnoreEntities.push_back(Owner);
    EXPECT_TRUE(Scene->CastRay(Ray()).has_value());
    EXPECT_FALSE(Scene->CastRay(Query).has_value());
    Scene->DestroyRagdoll(Handle);
    Scene->Update(0.0);
    EXPECT_EQ(Scene->GetBodyCount(), 0u);
}


// A sweep's radius reaches bodies the bare ray along the same line misses.
TEST_F(PhysicsSceneLifecycle, ShapeSweepsReachPastTheRayLine)
{
    const ECS::FEntity Entity = Spawn();
    Scene->Update(0.0);

    const FVector3 Start(0.6f, 0.0f, -3.0f);
    const FVector3 End(0.6f, 0.0f, 3.0f);
    EXPECT_FALSE(Scene->CastRay(Ray(0.6f)).has_value());

    for (const SQueryShape& Shape : { SQueryShape::Sphere(0.2f), SQueryShape::Box(FVector3(0.2f), FQuat::Identity()),
                                      SQueryShape::Capsule(0.2f, 0.5f, FQuat::Identity()) })
    {
        const TOptional<SRayResult> Hit = Scene->CastShapeClosest(Shape, Start, End, ECollisionProfiles::All, {});
        ASSERT_TRUE(Hit.has_value()) << "shape " << (int)Shape.Type;
        EXPECT_EQ(Hit->Entity, Entity.Value);
        EXPECT_LT(Hit->Location.z, 0.0f) << "the sweep touches the near side of the sphere";

        TVector<SRayResult> All;
        Scene->CastShapeAll(Shape, Start, End, ECollisionProfiles::All, {}, All);
        EXPECT_EQ(All.size(), 1u);
    }
}

TEST_F(PhysicsSceneLifecycle, AnyHitAndSingleEntityRays)
{
    const ECS::FEntity Near = Spawn(0.0f);
    const ECS::FEntity Far = Spawn(3.0f);
    Scene->Update(0.0);

    SRayCastSettings Along;
    Along.Start = FVector3(-5.0f, 0.0f, 0.0f);
    Along.End = FVector3(5.0f, 0.0f, 0.0f);
    EXPECT_TRUE(Scene->CastRayAny(Along));
    Along.IgnoreEntities.push_back(Near);
    Along.IgnoreEntities.push_back(Far);
    EXPECT_FALSE(Scene->CastRayAny(Along)) << "both blockers are ignored";

    const TOptional<SRayResult> Hit = Scene->CastRayAgainst(Far, Along.Start, Along.End);
    ASSERT_TRUE(Hit.has_value()) << "the near body in front does not block a ray aimed at one entity";
    EXPECT_EQ(Hit->Entity, Far.Value);
    EXPECT_NEAR(Hit->Location.x, 2.5f, 0.05f);
    EXPECT_FALSE(Scene->CastRayAgainst(Far, FVector3(-5.0f, 2.0f, 0.0f), FVector3(5.0f, 2.0f, 0.0f)).has_value());
}

TEST_F(PhysicsSceneLifecycle, OverlapsFollowShapeRotationAndLayerMask)
{
    const ECS::FEntity Entity = Spawn();
    Scene->Update(0.0);

    ECS::FEntity Found[4];
    const FVector3 Above(0.0f, 1.2f, 0.0f);
    EXPECT_EQ(Scene->OverlapShape(SQueryShape::Capsule(0.1f, 1.0f, FQuat::Identity()), Above, ECollisionProfiles::All, {}, Found), 1)
        << "a standing capsule reaches down into the sphere";
    EXPECT_EQ(Found[0], Entity);

    const FQuat Lying = Math::FromAxisAngle(FVector3(0.0f, 0.0f, 1.0f), Math::HalfPi<float>());
    EXPECT_EQ(Scene->OverlapShape(SQueryShape::Capsule(0.1f, 1.0f, Lying), Above, ECollisionProfiles::All, {}, Found), 0)
        << "the same capsule lying down stays above it";

    EXPECT_EQ(Scene->OverlapShape(SQueryShape::Sphere(1.0f), FVector3(0.0f), (ECollisionProfiles)0, {}, Found), 0)
        << "an empty layer mask matches nothing";
}

TEST_F(PhysicsSceneLifecycle, ClosestPointAndBoundsMeasureTheColliders)
{
    const ECS::FEntity Entity = Spawn();
    Scene->Update(0.0);

    FVector3 Closest;
    ASSERT_TRUE(Scene->GetClosestPointOnBody(Entity, FVector3(3.0f, 0.0f, 0.0f), Closest));
    EXPECT_NEAR(Closest.x, 0.5f, 0.01f);

    SAABB Bounds;
    ASSERT_TRUE(Scene->GetBodyBounds(Entity, Bounds));
    EXPECT_NEAR(Bounds.Min.x, -0.5f, 0.05f);
    EXPECT_NEAR(Bounds.Max.y, 0.5f, 0.05f);

    EXPECT_FALSE(Scene->GetBodyBounds(ECS::NullEntity, Bounds));
}
