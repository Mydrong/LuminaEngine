#pragma once

#include <thread>

#include "AI/Navigation/NavMesh.h"
#include "AI/Navigation/NavTypes.h"
#include "Containers/Function.h"
#include "Core/Object/ObjectBase.h"
#include "Core/Object/ObjectCore.h"
#include "Platform/Time/PlatformTime.h"
#include "World/World.h"
#include "World/Entity/Components/NavMeshComponent.h"
#include "World/Entity/Components/PhysicsComponent.h"
#include "World/Entity/Components/TransformComponent.h"
#include "World/Entity/Systems/NavMeshSystem.h"

namespace Lumina::NavTest
{
    inline void BindTransform(ECS::FRegistry& Registry, ECS::FEntity Entity)
    {
        STransformComponent& Transform = Registry.Get<STransformComponent>(Entity);
        Transform.Bind(Registry, Entity);
        Transform.ResetDirtyState();
    }

    inline ECS::FEntity SpawnBox(ECS::FRegistry& Registry, const FVector3& Location, const FVector3& Half)
    {
        const ECS::FEntity Entity = Registry.Create();
        FTransform Transform;
        Transform.SetLocation(Location);
        Registry.Emplace<STransformComponent>(Entity, Transform);
        Registry.Emplace<SBoxColliderComponent>(Entity).HalfExtent = Half;
        return Entity;
    }

    // A level as the engine sees it, colliders in a registry and a nav volume ticked by the real system.
    struct FNavWorld
    {
        CWorld*         World = nullptr;
        SNavMeshSystem* System = nullptr;
        ECS::FEntity    Volume;

        ECS::FRegistry& Registry() const { return ECS::GetWorldRegistry(*World); }
        SNavMeshComponent& Nav() const { return Registry().Get<SNavMeshComponent>(Volume); }

        // A world with only ground under the volume, square and Size across.
        void Create(float Size, float GroundSlab = 256.0f)
        {
            RegisterPendingTypes();
            World = NewObject<CWorld>(nullptr, FName("NavTestWorld"), FGuid::New(), OF_Transient);
            ECS::FRegistry& R = Registry();
            R.GetSignals<STransformComponent>().OnConstruct.Connect<&BindTransform>();

            const float Slab = Math::Min(GroundSlab, Size);
            const int32 Slabs = Math::Max(1, (int32)(Size / Slab));
            for (int32 Z = 0; Z < Slabs; ++Z)
            {
                for (int32 X = 0; X < Slabs; ++X)
                {
                    SpawnBox(R, FVector3(-Size * 0.5f + (X + 0.5f) * Slab, -0.5f, -Size * 0.5f + (Z + 0.5f) * Slab), FVector3(Slab * 0.5f, 0.5f, Slab * 0.5f));
                }
            }

            Volume = R.Create();
            R.Emplace<STransformComponent>(Volume);
            SNavMeshComponent& Mesh = R.Emplace<SNavMeshComponent>(Volume);
            Mesh.Center = FVector3(0.0f, 2.0f, 0.0f);
            Mesh.Extents = FVector3(Size * 0.5f, 8.0f, Size * 0.5f);
            Mesh.bAutoBake = false;
            Mesh.bDynamicRebuild = true;
            Mesh.DynamicRebuildInterval = 0.0f;
        }

        void Start()
        {
            Nav().bBakeRequested = true;
            System = NewObject<SNavMeshSystem>(nullptr, NAME_None, FGuid::New(), OF_Transient);
            System->SetOwningWorld(World);
            World->Systems.push_back(System);
            System->OnStartup();
        }

        void Stop()
        {
            if (System != nullptr)
            {
                System->OnTeardown();
                System = nullptr;
            }
        }

        double Tick()
        {
            PlatformTime::FStopwatch Watch;
            System->OnUpdate();
            return Watch.ElapsedMilliseconds();
        }

        bool TickUntil(const TFunction<bool()>& Done, double TimeoutMs)
        {
            PlatformTime::FStopwatch Watch;
            while (!Done())
            {
                if (Watch.ElapsedMilliseconds() > TimeoutMs)
                {
                    return false;
                }
                Tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            return true;
        }

        bool WaitReady(double TimeoutMs = 60000.0)
        {
            return TickUntil([this] { return Nav().Runtime.State == ENavBakeState::Ready; }, TimeoutMs);
        }

        // Settled once nothing is dirty, settling or rebaking, after a couple of scans have had their look.
        bool WaitSettled(double TimeoutMs = 60000.0)
        {
            Tick();
            Tick();
            return TickUntil([this]
            {
                const FNavMeshRuntime& Runtime = Nav().Runtime;
                return Runtime.DirtyTiles.empty() && Runtime.SettlingTiles.empty() && Runtime.PendingRebakes.empty() && Runtime.PendingSourceChanges.empty();
            }, TimeoutMs);
        }

        // True where an agent could stand on the ground itself, rather than on something set down there.
        bool IsGroundWalkable(float X, float Z) const
        {
            FVector3 Out;
            FNavQueryFilter Filter;
            // The nearest polygon can be the ground beside an obstacle, so the answer has to stay where it was asked.
            return Nav().Runtime.Mesh && Nav().Runtime.Mesh->ProjectPoint(FVector3(X, 0.0f, Z), FVector3(0.1f, 0.3f, 0.1f), Filter, Out)
                && std::fabs(Out.y) < 0.3f && std::fabs(Out.x - X) < 0.05f && std::fabs(Out.z - Z) < 0.05f;
        }
    };
}
