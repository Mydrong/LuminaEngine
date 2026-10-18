#pragma once

#include "Containers/Name.h"
#include "Containers/String.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "GameLibrary.generated.h"

namespace Lumina
{
    // Process level session operations, where a level change lives on CWorld since editor PIE runs one world per player.
    REFLECT()
    class RUNTIME_API CGameLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** Ends the play session in the editor and exits the process in a packaged game, both deferred. */
        FUNCTION()
        static void QuitGame();

        // True for a dedicated server or load-test process, which never opens a window or an audio device.
        FUNCTION()
        static bool IsHeadless();
    };
}
