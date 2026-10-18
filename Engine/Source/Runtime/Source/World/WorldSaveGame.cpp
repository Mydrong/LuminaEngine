#include "RuntimePCH.h"

#include "SaveGame/SaveGameEvents.h"
#include "World/ECS/EventQueue.h"
#include "World/World.h"

namespace Lumina
{
    void CWorld::RequestSaveGame(CSaveGame* SaveGame, const FString& SlotName, int32 UserIndex)
    {
        SSaveGameRequestEvent Request;
        Request.Action    = ESaveGameAction::Save;
        Request.SlotName  = SlotName;
        Request.UserIndex = UserIndex;
        Request.SaveGame  = SaveGame;
        ECS::SendEvent(GetEvents(), Request);
    }

    void CWorld::RequestLoadGame(const FString& SlotName, int32 UserIndex)
    {
        SSaveGameRequestEvent Request;
        Request.Action    = ESaveGameAction::Load;
        Request.SlotName  = SlotName;
        Request.UserIndex = UserIndex;
        ECS::SendEvent(GetEvents(), Request);
    }
}
