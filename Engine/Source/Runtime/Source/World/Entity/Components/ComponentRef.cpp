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
        if (*EpochSlot == Epoch)
        {
            return Address;
        }
        if (*EpochSlot == ECS::FSparseSet::DeadLayoutEpoch)
        {
            return nullptr;
        }

        // The pool moved or lost a member, so the component is found again and its new address kept.
        ECS::FRegistry& Registry = ECS::GetWorldRegistry(*reinterpret_cast<CWorld*>(World));
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

        ECS::FRegistry& Registry = ECS::GetWorldRegistry(InWorld);
        void* Found = bAdd ? InOps.Emplace(Registry, InEntity) : InOps.Get(Registry, InEntity);
        const ECS::FSparseSet* Pool = Found != nullptr ? Registry.FindStorage(static_cast<uint32>(InOps.TypeId)) : nullptr;
        if (Pool == nullptr)
        {
            return;
        }

        Address   = Found;
        EpochSlot = Pool->GetLayoutEpoch();
        Epoch     = *EpochSlot;
        World     = reinterpret_cast<uint64>(&InWorld);
        Ops       = &InOps;
        Entity    = InEntity.GetPacked();
    }
}
