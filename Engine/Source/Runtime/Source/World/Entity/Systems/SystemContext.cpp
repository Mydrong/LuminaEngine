#include "RuntimePCH.h"
#include "SystemContext.h"
#include "World/ECS/Registry.h"
#include "World/World.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/LifetimeComponent.h"
#include "World/Entity/Components/NameComponent.h"

namespace Lumina
{
    namespace
    {
        // Honest-access checks for the access-implying context helpers (no-op in Shipping / outside a system).
        void CheckTransformWrite()
        {
            ValidateSystemAccess(static_cast<uint32>(ECS::GetComponentTypeID<STransformComponent>()), true, "Write<STransformComponent>");
        }
        void CheckPhysics(bool bWrite)
        {
            ValidateSystemAccess(static_cast<uint32>(ECS::GetComponentTypeID<SystemResource::PhysicsQuery>()), bWrite,
                bWrite ? "Write<SystemResource::PhysicsQuery>" : "Read<SystemResource::PhysicsQuery>");
        }
        void CheckHierarchyRead()
        {
            ValidateSystemAccess(static_cast<uint32>(ECS::GetComponentTypeID<SystemResource::Hierarchy>()), false, "Read<SystemResource::Hierarchy>");
        }
        void CheckStructure()
        {
            ValidateSystemAccess(static_cast<uint32>(ECS::GetComponentTypeID<SystemResource::EntityStructure>()), true, "Write<SystemResource::EntityStructure>");
        }
    }

    SSystemContext::SSystemContext(CWorld* InWorld)
        : World(InWorld)
        , Registry(&InWorld->EntityRegistry)
        , Events(&InWorld->EventQueues)
        , CommandBus(&InWorld->CommandBus)
    {}


    const ECS::FHierarchy& SSystemContext::GetHierarchy() const
    {
        CheckHierarchyRead();
        return Registry->GetHierarchy();
    }

    void SSystemContext::SetEntityLifetime(ECS::FEntity Entity, float Lifetime) const
    {
        Registry->GetOrEmplace<SLifetimeComponent>(Entity).Lifetime = Lifetime;
    }

    // Read, not write, since a declared Write satisfies it and only a total absence is caught.
    Physics::IPhysicsScene* SSystemContext::GetPhysicsScene() const
    {
        CheckPhysics(false);
        return World ? World->GetPhysicsScene() : nullptr;
    }

    void SSystemContext::ActivateBody(ECS::FEntity Entity)
    {
        CheckPhysics(true);
        World->PhysicsScene->ActivateBody(Entity);
    }

    void SSystemContext::DeactivateBody(ECS::FEntity Entity)
    {
        CheckPhysics(true);
        World->PhysicsScene->DeactivateBody(Entity);
    }

    void SSystemContext::ChangeBodyMotionType(ECS::FEntity Entity, EBodyType NewType)
    {
        CheckPhysics(true);
        World->PhysicsScene->ChangeBodyMotionType(Entity, NewType);
    }

    FVector3 SSystemContext::GetBodyPosition(ECS::FEntity Entity) const
    {
        CheckPhysics(false);
        return World->PhysicsScene ? World->PhysicsScene->GetBodyPosition(Entity) : FVector3(0.0f);
    }

    FQuat SSystemContext::GetBodyRotation(ECS::FEntity Entity) const
    {
        CheckPhysics(false);
        return World->PhysicsScene ? World->PhysicsScene->GetBodyRotation(Entity) : FQuat();
    }

    FVector3 SSystemContext::GetVelocityAtPoint(ECS::FEntity Entity, const FVector3& Point) const
    {
        CheckPhysics(false);
        return World->PhysicsScene ? World->PhysicsScene->GetVelocityAtPoint(Entity, Point) : FVector3(0.0f);
    }

    void SSystemContext::AddForceAtPosition(ECS::FEntity Entity, const FVector3& Force, const FVector3& Position) const
    {
        CheckPhysics(true);
        if (World->PhysicsScene)
        {
            World->PhysicsScene->AddForceAtPosition(Entity, Force, Position);
        }
    }

    void SSystemContext::ApplyBuoyancyImpulse(ECS::FEntity Entity, const FVector3& SurfacePosition, const FVector3& SurfaceNormal,
        float Buoyancy, float LinearDrag, float AngularDrag, const FVector3& FluidVelocity, float InDeltaTime) const
    {
        CheckPhysics(true);
        if (World->PhysicsScene)
        {
            World->PhysicsScene->ApplyBuoyancyImpulse(Entity, SurfacePosition, SurfaceNormal, Buoyancy, LinearDrag, AngularDrag, FluidVelocity, InDeltaTime);
        }
    }


    void SSystemContext::CastSphere(const SSphereCastSettings& Settings, TVector<SRayResult>& OutHits) const
    {
        CheckPhysics(false);
        OutHits.clear();
        if (World->PhysicsScene)
        {
            World->PhysicsScene->CastSphere(Settings, OutHits);
        }
    }

    TOptional<SRayResult> SSystemContext::CastSphereClosest(const SSphereCastSettings& Settings) const
    {
        CheckPhysics(false);
        return World->PhysicsScene ? World->PhysicsScene->CastSphereClosest(Settings) : NullOpt;
    }

    STransformComponent& SSystemContext::GetEntityTransform(ECS::FEntity Entity) const
    {
        CheckTransformWrite(); // returns a mutable ref -> caller may write
        return Get<STransformComponent>(Entity);
    }

    FVector3 SSystemContext::TranslateEntity(ECS::FEntity Entity, const FVector3& Translation)
    {
        CheckTransformWrite();
        FVector3 NewLocation = Registry->Get<STransformComponent>(Entity).Translate(Translation);
        return NewLocation;
    }

    void SSystemContext::SetEntityLocation(ECS::FEntity Entity, const FVector3& Location)
    {
        CheckTransformWrite();
        Registry->Get<STransformComponent>(Entity).SetLocation(Location);
    }

    void SSystemContext::SetEntityRotation(ECS::FEntity Entity, const FQuat& Rotation)
    {
        CheckTransformWrite();
        Registry->Get<STransformComponent>(Entity).SetRotation(Rotation);
    }

    void SSystemContext::SetEntityScale(ECS::FEntity Entity, const FVector3& Scale)
    {
        CheckTransformWrite();
        Registry->Get<STransformComponent>(Entity).SetScale(Scale);
    }

    void SSystemContext::DrawDebugLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float Thickness, float Duration) const
    {
        World->GetDebugInterface()->DrawLine(Start, End, Color, Thickness, true, Duration);
    }

    void SSystemContext::DrawDebugBox(const FVector3& Center, const FVector3& Extents, const FQuat& Rotation, const FVector4& Color, float Thickness, float Duration) const
    {
        World->GetDebugInterface()->DrawBox(Center, Extents, Rotation, Color, Thickness, true, Duration);
    }

    void SSystemContext::DrawDebugSphere(const FVector3& Center, float Radius, const FVector4& Color, uint8 Segments, float Thickness, float Duration) const
    {
        World->GetDebugInterface()->DrawSphere(Center, Radius, Color, Segments, Thickness, true, Duration);
    }

    void SSystemContext::DrawDebugCone(const FVector3& Apex, const FVector3& Direction, float AngleRadians, float Length, const FVector4& Color, uint8 Segments, uint8 Stacks, float Thickness, float Duration) const
    {
        World->GetDebugInterface()->DrawCone(Apex, Direction, AngleRadians, Length, Color, Segments, Stacks, Thickness, true, Duration);
    }

    void SSystemContext::DrawFrustum(const FMatrix4& Matrix, float zNear, float zFar, const FVector4& Color, float Thickness, float Duration) const
    {
        World->GetDebugInterface()->DrawFrustum(Matrix, zNear, zFar, Color, Thickness, true, Duration);
    }

    void SSystemContext::DrawDebugArrow(const FVector3& Start, const FVector3& Direction, float Length, const FVector4& Color, float Thickness, float Duration, float HeadSize) const
    {
        World->GetDebugInterface()->DrawArrow(Start, Direction, Length, Color, Thickness, true, Duration, HeadSize);
    }

    void SSystemContext::DrawDebugSolidTriangles(TVector<FSimpleElementVertex>&& Vertices, ESolidDrawMode Mode, float Duration) const
    {
        World->GetDebugInterface()->DrawSolidTriangles(std::move(Vertices), Mode, Duration);
    }

    ECS::FEntity SSystemContext::Create(const FTransform& Transform, FName EntityName) const
    {
        LUMINA_PROFILE_SCOPE();
        CheckStructure();

        ECS::FEntity EntityID = Registry->Create();
        Registry->Emplace<STransformComponent>(EntityID).SetWorldTransform(Transform);
        Registry->Emplace<SNameComponent>(EntityID, EntityName);
        Registry->EmplaceOrReplace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }
    
    ECS::FEntity SSystemContext::Create(FVector3 Location, FName EntityName) const
    {
        LUMINA_PROFILE_SCOPE();
        CheckStructure();

        ECS::FEntity EntityID = Registry->Create();
        Registry->Emplace<STransformComponent>(EntityID).SetLocation(Location);
        Registry->Emplace<SNameComponent>(EntityID, EntityName);
        Registry->EmplaceOrReplace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }
    
    ECS::FEntity SSystemContext::Create(FName EntityName) const
    {
        LUMINA_PROFILE_SCOPE();
        CheckStructure();

        ECS::FEntity EntityID = Registry->Create();
        Registry->Emplace<STransformComponent>(EntityID);
        Registry->Emplace<SNameComponent>(EntityID, EntityName);
        Registry->EmplaceOrReplace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }

    ECS::FEntity SSystemContext::CreateDeferred(const FTransform& Transform, FName EntityName) const
    {
        const ECS::FEntity EntityID = CommandBus->Create();
        CommandBus->Enqueue([EntityID, Transform, EntityName](ECS::FRegistry& InRegistry)
        {
            if (!InRegistry.IsValid(EntityID))
            {
                return;
            }
            InRegistry.EmplaceOrReplace<STransformComponent>(EntityID).SetWorldTransform(Transform);
            InRegistry.EmplaceOrReplace<SNameComponent>(EntityID, EntityName);
            InRegistry.EmplaceOrReplace<FNeedsTransformUpdate>(EntityID);
        });
        return EntityID;
    }

    size_t SSystemContext::GetNumEntities() const
    {
        return Registry->NumEntities();
    }

    bool SSystemContext::IsValidEntity(ECS::FEntity Entity) const
    {
        return Registry->IsValid(Entity);
    }

    EWorldType SSystemContext::GetWorldType() const
    {
        return World->GetWorldType();
    }
}
