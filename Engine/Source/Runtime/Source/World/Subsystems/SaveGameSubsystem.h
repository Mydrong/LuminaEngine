#pragma once

#include "Containers/String.h"
#include "Containers/Vector.h"
#include "Core/Object/ObjectMacros.h"
#include "World/Subsystems/WorldSubsystem.h"
#include "SaveGameSubsystem.generated.h"

namespace Lumina
{
    class CSaveGame;

    // Save and load requests, served by the save game system.
    REFLECT()
    class RUNTIME_API CSaveGameSubsystem : public CWorldSubsystem
    {
        GENERATED_BODY()

    public:

        // Captures World into SaveGame and writes it at the end of the frame, safe from any system or thread.
        FUNCTION()
        void RequestSaveGame(CSaveGame* SaveGame, const FString& SlotName, int32 UserIndex);

        // Loads the slot and restores World at the end of the frame. SSaveGameCompletedEvent and OnWorldRestored report back.
        FUNCTION()
        void RequestLoadGame(const FString& SlotName, int32 UserIndex);
    };
}
