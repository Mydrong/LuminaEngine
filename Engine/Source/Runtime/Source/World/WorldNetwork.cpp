#include "RuntimePCH.h"

#include "World/World.h"

#include "Networking/INetworkRuntime.h"

namespace Lumina
{
    uint32 CWorld::GetLocalNetConnection()
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr ? Net->GetLocalConnectionId(this) : 0u;
    }

    void CWorld::GetNetConnections(TVector<uint32>& OutConnections)
    {
        OutConnections.clear();
        if (INetworkRuntime* Net = GetNetworkRuntime(); Net != nullptr)
        {
            Net->GetConnectionIds(this, OutConnections);
        }
    }

    bool CWorld::IsEntityNetworked(ECS::FEntity Entity)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && Net->IsEntityNetworked(this, Entity);
    }

    uint32 CWorld::GetNetOwner(ECS::FEntity Entity)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr ? Net->GetEntityOwner(this, Entity) : 0u;
    }

    bool CWorld::SetNetOwner(ECS::FEntity Entity, uint32 ConnectionId)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && Net->SetEntityOwner(this, Entity, ConnectionId);
    }

    void CWorld::MarkEntityNetDirty(ECS::FEntity Entity)
    {
        if (INetworkRuntime* Net = GetNetworkRuntime(); Net != nullptr)
        {
            Net->MarkEntityDirty(this, Entity);
        }
    }

    uint32 CWorld::EntityToNetId(ECS::FEntity Entity)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr ? Net->EntityToNetId(this, Entity) : 0u;
    }

    ECS::FEntity CWorld::NetIdToEntity(uint32 NetId)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr ? Net->NetIdToEntity(this, NetId) : ECS::NullEntity;
    }

    bool CWorld::IsNetJoined()
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && Net->IsJoined(this);
    }

    ECS::FEntity CWorld::GetOwnedPawn(uint32 ConnectionId)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr ? Net->FindOwnedPawn(this, ConnectionId) : ECS::NullEntity;
    }
}
