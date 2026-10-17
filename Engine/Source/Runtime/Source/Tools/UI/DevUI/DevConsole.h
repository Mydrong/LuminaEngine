#pragma once

#include "Containers/String.h"
#include "Containers/Vector.h"

struct ImGuiInputTextCallbackData;

namespace Lumina
{
    // The game overlay's console, a log tail over a command line with history and tab completion of console names.
    class FDevConsole
    {
    public:

        // Draws the console docked along the top of the window, with the overlay's setting for script windows in its header.
        void Draw(bool bFocusInput, bool& bKeepScriptWindowsVisible);

    private:

        void DrawLog();
        void DrawInput(bool bFocusInput);
        void DrawSuggestions();
        void Submit();

        static int InputCallback(ImGuiInputTextCallbackData* Data);
        int HandleInputCallback(ImGuiInputTextCallbackData* Data);

        // Console variable and command names starting with Prefix, ignoring case, sorted.
        void GatherMatches(FStringView Prefix, TVector<FString>& Out) const;

        static constexpr int32 kInputCapacity = 256;

        char             InputBuffer[kInputCapacity] = {};
        TVector<FString> History;
        // An index into History while walking it with the arrow keys, or -1 at a fresh line.
        int32            HistoryPosition = -1;
        bool             bScrollToBottom = true;
    };
}
