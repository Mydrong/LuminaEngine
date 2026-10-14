#pragma once

#include "Containers/Name.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Scripting/ScriptCallback.h"
#include "World/ECS/Entity.h"
#include "GameplayMessageLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    // The message bus as script reaches it; typed sending and listening is built over these in C#.
    REFLECT()
    class RUNTIME_API CGameplayMessageLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        // The key C++ and C# senders share for a message type name.
        FUNCTION(ScriptInternal)
        static uint64 GetMessageTypeKey(FName TypeName);

        // The bus owns Listener from here, and releases it when the subscription goes.
        FUNCTION(ScriptInternal)
        static uint64 Subscribe(CWorld* World, uint32 Channel, ECS::FEntity Owner, uint64 Type, bool bExactMatch, SScriptCallback Listener);

        FUNCTION(ScriptInternal)
        static bool Unsubscribe(CWorld* World, uint64 Id);

        FUNCTION(ScriptInternal)
        static void UnsubscribeAll(CWorld* World, ECS::FEntity Owner);

        // A managed payload's handle belongs to the bus once sent, whether or not anything receives it.
        FUNCTION(ScriptInternal)
        static void Send(CWorld* World, uint32 Route, ECS::FEntity Target, uint32 Channel, uint32 Kind, uint64 Type,
            void* Data, uint32 Size, void* Struct, bool bIncludeSelf);
    };
}
