#pragma once

#include "Containers/Vector.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "World/ECS/Entity.h"
#include "NetworkLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    // The session a world is part of, answering as a standalone world when no netcode is installed.
    REFLECT()
    class RUNTIME_API CNetworkLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        // This peer's own connection, which is 0 on a host and in a standalone world.
        FUNCTION()
        static uint32 GetLocalConnection(CWorld* World);

        // The remote connections that finished joining, which only a host can see.
        FUNCTION()
        static void GetConnections(CWorld* World, TVector<uint32>& OutConnections);

        // Needs an SNetworkComponent on the entity.
        FUNCTION()
        static bool IsEntityNetworked(CWorld* World, ECS::FEntity Entity);

        FUNCTION()
        static uint32 GetOwner(CWorld* World, ECS::FEntity Entity);

        // Host only, a client asking is refused.
        FUNCTION()
        static bool SetOwner(CWorld* World, ECS::FEntity Entity, uint32 ConnectionId);

        // Resends the entity's replicated component fields.
        FUNCTION()
        static void MarkEntityDirty(CWorld* World, ECS::FEntity Entity);

        FUNCTION()
        static uint32 EntityToNetId(CWorld* World, ECS::FEntity Entity);

        FUNCTION()
        static ECS::FEntity NetIdToEntity(CWorld* World, uint32 NetId);

        // A host is always joined, and a client is once its handshake finished.
        FUNCTION()
        static bool IsJoined(CWorld* World);

        // The entity a connection owns, preferring one with a character controller.
        FUNCTION()
        static ECS::FEntity GetOwnedPawn(CWorld* World, uint32 ConnectionId);
    };
}
