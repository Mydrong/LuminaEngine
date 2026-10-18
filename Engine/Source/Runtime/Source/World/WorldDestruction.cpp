#include "RuntimePCH.h"

#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "World/Entity/Components/DestructibleComponent.h"
#include "World/Entity/Components/StaticMeshComponent.h"
#include "World/World.h"

namespace Lumina
{
    bool CWorld::ShatterMesh(CStaticMesh* Mesh, FTransform Transform, FVector3 Origin, float Strength,
                                          int32 Pieces, float Lifetime)
    {
        if (Mesh == nullptr)
        {
            return false;
        }

        const ECS::FEntity Entity = ConstructEntity("Shattered", Transform);
        SStaticMeshComponent& MeshComponent = EmplaceComponent<SStaticMeshComponent>(Entity);
        MeshComponent.StaticMesh = Mesh;

        SDestructibleComponent& Destructible = EmplaceComponent<SDestructibleComponent>(Entity);
        Destructible.FragmentCount = Math::Max(Pieces, 2);
        Destructible.FragmentLifetime = Math::Max(Lifetime, 0.1f);
        Destructible.bDestroyOriginal = true;

        if (!FractureEntity(Entity, Origin, Strength))
        {
            DestroyEntity(Entity);
            return false;
        }
        return true;
    }
}
