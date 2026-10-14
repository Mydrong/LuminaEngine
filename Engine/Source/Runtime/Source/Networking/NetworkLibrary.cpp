#include "RuntimePCH.h"
#include "NetworkLibrary.h"

#include "Networking/INetworkRuntime.h"

namespace Lumina
{
    uint32 CNetworkLibrary::GetLocalConnection(CWorld* World)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr ? Net->GetLocalConnectionId(World) : 0u;
    }

    void CNetworkLibrary::GetConnections(CWorld* World, TVector<uint32>& OutConnections)
    {
        OutConnections.clear();
        if (INetworkRuntime* Net = GetNetworkRuntime(); Net != nullptr && World != nullptr)
        {
            Net->GetConnectionIds(World, OutConnections);
        }
    }

    bool CNetworkLibrary::IsEntityNetworked(CWorld* World, ECS::FEntity Entity)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr && Net->IsEntityNetworked(World, Entity);
    }

    uint32 CNetworkLibrary::GetOwner(CWorld* World, ECS::FEntity Entity)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr ? Net->GetEntityOwner(World, Entity) : 0u;
    }

    bool CNetworkLibrary::SetOwner(CWorld* World, ECS::FEntity Entity, uint32 ConnectionId)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr && Net->SetEntityOwner(World, Entity, ConnectionId);
    }

    void CNetworkLibrary::MarkEntityDirty(CWorld* World, ECS::FEntity Entity)
    {
        if (INetworkRuntime* Net = GetNetworkRuntime(); Net != nullptr && World != nullptr)
        {
            Net->MarkEntityDirty(World, Entity);
        }
    }

    uint32 CNetworkLibrary::EntityToNetId(CWorld* World, ECS::FEntity Entity)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr ? Net->EntityToNetId(World, Entity) : 0u;
    }

    ECS::FEntity CNetworkLibrary::NetIdToEntity(CWorld* World, uint32 NetId)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr ? Net->NetIdToEntity(World, NetId) : ECS::NullEntity;
    }

    bool CNetworkLibrary::IsJoined(CWorld* World)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr && Net->IsJoined(World);
    }

    ECS::FEntity CNetworkLibrary::GetOwnedPawn(CWorld* World, uint32 ConnectionId)
    {
        INetworkRuntime* Net = GetNetworkRuntime();
        return Net != nullptr && World != nullptr ? Net->FindOwnedPawn(World, ConnectionId) : ECS::NullEntity;
    }
}
