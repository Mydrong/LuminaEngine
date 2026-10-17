#pragma once

#include "Containers/Name.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "EventQueueLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    // A world's event queues as script reaches them, with the typed reading and sending built over these in C#.
    REFLECT()
    class RUNTIME_API CEventQueueLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        // The queue type for a reflected event of that name, or a script event laid out as described. 0xFFFF when neither fits.
        FUNCTION(ScriptInternal)
        static uint32 ResolveEventType(FName Name, uint32 Size, uint32 Alignment);

        FUNCTION(ScriptInternal)
        static void Send(CWorld* World, uint32 TypeId, void* Events, uint32 Count);

        // Fills Runs, laid out as ECS::FEventRuns, with what is unread past the uint64 at Cursor, and moves the cursor past it.
        FUNCTION(ScriptInternal)
        static void Read(CWorld* World, uint32 TypeId, void* Cursor, void* Runs);

        FUNCTION(ScriptInternal)
        static uint32 NumUnread(CWorld* World, uint32 TypeId, uint64 Cursor);
    };
}
