#include "RuntimePCH.h"
#include "ComponentRef.h"

#include "World/World.h"
#include "World/ECS/Registry.h"
#include "World/ECS/SparseSet.h"
#include "World/Entity/Components/Component.h"

namespace Lumina
{
    void* FComponentRef::Resolve() const
    {
        if (EpochSlot == nullptr)
        {
            return nullptr;
        }
        if (*EpochSlot == Epoch && Address != nullptr)
        {
            return Address;
        }
        if (*EpochSlot == ECS::FSparseSet::DeadLayoutEpoch)
        {
            return nullptr;
        }

        // The pool moved or lost a member, so the component is found again and its new address kept.
        ECS::FRegistry& Registry = reinterpret_cast<CWorld*>(World)->GetEntityRegistry();
        void* Found = Ops->Get(Registry, ECS::FEntity(Entity));
        if (Found == nullptr)
        {
            return nullptr;
        }
        Address = Found;
        Epoch   = *EpochSlot;
        return Address;
    }

    void FComponentRef::Bind(CWorld& InWorld, ECS::FEntity InEntity, const FComponentOps& InOps, bool bAdd)
    {
        Reset();

        ECS::FRegistry& Registry = InWorld.GetEntityRegistry();
        void* Found = bAdd ? InOps.Emplace(Registry, InEntity) : InOps.Get(Registry, InEntity);

        // Tracked even while missing, so a component a prefab or sibling adds after the bind is found on the next read.
        const ECS::FSparseSet& Pool = Registry.AssureStorage(InOps.GetTypeID());

        Address   = Found;
        EpochSlot = Pool.GetLayoutEpoch();
        Epoch     = *EpochSlot;
        World     = reinterpret_cast<uint64>(&InWorld);
        Ops       = &InOps;
        Entity    = InEntity.GetPacked();
    }
}
