#include "RuntimePCH.h"

#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        SFoliageComponent* FindFoliage(CWorld& World, ECS::FEntity& OutEntity)
        {
            auto View = World.View<SFoliageComponent>();
            for (ECS::FEntity Entity : View)
            {
                OutEntity = Entity;
                return &View.Get<SFoliageComponent>(Entity);
            }

            return nullptr;
        }

        SFoliageComponent* FindOrCreateFoliage(CWorld& World, ECS::FEntity& OutEntity)
        {
            if (SFoliageComponent* Existing = FindFoliage(World, OutEntity))
            {
                return Existing;
            }

            OutEntity = World.ConstructEntity("Foliage");
            return &World.EmplaceComponent<SFoliageComponent>(OutEntity);
        }
    }

    ECS::FEntity CWorld::GetOrCreateFoliage()
    {
        ECS::FEntity Entity = ECS::NullEntity;
        {
            FindOrCreateFoliage(*this, Entity);
        }
        return Entity;
    }

    int32 CWorld::AddFoliageType(CStaticMesh* Mesh, bool bCastShadow, float CullDistance)
    {
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = FindOrCreateFoliage(*this, Entity);
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

        MarkFoliageChanged(*this, Entity, *Foliage);
        return (int32)Foliage->Types.size() - 1;
    }

    void CWorld::AddFoliageInstances(const SFoliageInstance* Instances, int32 Count)
    {
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = FindOrCreateFoliage(*this, Entity);
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

        MarkFoliageChanged(*this, Entity, *Foliage);
    }

    int32 CWorld::RemoveFoliageInRadius(FVector3 Center, float Radius, int32 TypeFilter)
    {
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = FindFoliage(*this, Entity);
        if (Foliage == nullptr)
        {
            return 0;
        }

        const int32 Removed = Foliage->RemoveInRadius(Center, Radius, TypeFilter);
        if (Removed > 0)
        {
            MarkFoliageChanged(*this, Entity, *Foliage);
        }
        return Removed;
    }

    void CWorld::SetFoliageInstanceHidden(int32 Index, bool bHidden)
    {
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = FindFoliage(*this, Entity);
        if (Foliage != nullptr && Index >= 0)
        {
            Foliage->SetInstanceHidden((uint32)Index, bHidden);
        }
    }

    int32 CWorld::GetFoliageInstanceCount()
    {
        ECS::FEntity Entity = ECS::NullEntity;
        SFoliageComponent* Foliage = FindFoliage(*this, Entity);
        return Foliage != nullptr ? (int32)Foliage->Instances.size() : 0;
    }
}
