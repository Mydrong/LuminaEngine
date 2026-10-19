#include "RuntimePCH.h"
#include "World/Subsystems/EnvironmentSubsystem.h"

#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        SFoliageComponent* FindFoliage(CWorld* World, ECS::FEntity& OutEntity)
        {
            auto View = World->View<SFoliageComponent>();
            for (ECS::FEntity Entity : View)
            {
                OutEntity = Entity;
                return &View.Get<SFoliageComponent>(Entity);
            }

            return nullptr;
        }

        SFoliageComponent* FindOrCreateFoliage(CWorld* World, ECS::FEntity& OutEntity)
        {
            if (SFoliageComponent* Existing = FindFoliage(World, OutEntity))
            {
                return Existing;
            }

            OutEntity = World->ConstructEntity("Foliage");
            return &World->EmplaceComponent<SFoliageComponent>(OutEntity);
        }
    }

    ECS::FEntity CEnvironmentSubsystem::GetOrCreateFoliage()
    {
        CWorld* World = GetWorld();
        ECS::FEntity Entity = ECS::NullEntity;
        if (World != nullptr)
        {
            FindOrCreateFoliage(World, Entity);
        }
        return Entity;
    }

    int32 CEnvironmentSubsystem::AddFoliageType(CStaticMesh* Mesh, bool bCastShadow, float CullDistance)
    {
        CWorld* World = GetWorld();
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = World != nullptr ? FindOrCreateFoliage(World, Entity) : nullptr;
        if (Foliage == nullptr || Mesh == nullptr)
        {
            return -1;
        }

        SFoliageType& Type = Foliage->Types.emplace_back();
        Type.Name           = Mesh->GetName().ToString();
        Type.Mesh           = Mesh;
        Type.bCastShadow    = bCastShadow;
        Type.CullDistance   = CullDistance;
        Type.bFollowTerrain = false;

        MarkFoliageChanged(*World, Entity, *Foliage);
        return (int32)Foliage->Types.size() - 1;
    }

    void CEnvironmentSubsystem::AddFoliageInstances(const SFoliageInstance* Instances, int32 Count)
    {
        CWorld* World = GetWorld();
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = World != nullptr ? FindOrCreateFoliage(World, Entity) : nullptr;
        if (Foliage == nullptr || Instances == nullptr || Count <= 0)
        {
            return;
        }

        Foliage->Instances.reserve(Foliage->Instances.size() + Count);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            if (Foliage->IsValidType(Instances[Index].TypeIndex))
            {
                Foliage->Instances.push_back(Instances[Index]);
            }
        }

        MarkFoliageChanged(*World, Entity, *Foliage);
    }

    int32 CEnvironmentSubsystem::RemoveFoliageInRadius(FVector3 Center, float Radius, int32 TypeFilter)
    {
        CWorld* World = GetWorld();
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = World != nullptr ? FindFoliage(World, Entity) : nullptr;
        if (Foliage == nullptr)
        {
            return 0;
        }

        const int32 Removed = Foliage->RemoveInRadius(Center, Radius, TypeFilter);
        if (Removed > 0)
        {
            MarkFoliageChanged(*World, Entity, *Foliage);
        }
        return Removed;
    }

    void CEnvironmentSubsystem::SetFoliageInstanceHidden(int32 Index, bool bHidden)
    {
        CWorld* World = GetWorld();
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = World != nullptr ? FindFoliage(World, Entity) : nullptr;
        if (Foliage != nullptr && Index >= 0)
        {
            Foliage->SetInstanceHidden((uint32)Index, bHidden);
        }
    }

    int32 CEnvironmentSubsystem::GetFoliageInstanceCount()
    {
        CWorld* World = GetWorld();
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = World != nullptr ? FindFoliage(World, Entity) : nullptr;
        return Foliage != nullptr ? (int32)Foliage->Instances.size() : 0;
    }
}
