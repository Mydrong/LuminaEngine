#pragma once

#include "Core/Object/ObjectHandleTyped.h"
#include "Events/MouseCodes.h"
#include "Input/InputMode.h"

namespace Lumina
{
    class CWorld;

    // Releases its lease when destroyed, for C++ code that holds a mode for a scope or an object's lifetime.
    class RUNTIME_API FScopedInputMode
    {
    public:

        FScopedInputMode() = default;
        FScopedInputMode(CWorld* InWorld, EInputMode InputMode, EMouseMode MouseMode);
        ~FScopedInputMode() { Reset(); }

        FScopedInputMode(FScopedInputMode&& Other) noexcept;
        FScopedInputMode& operator=(FScopedInputMode&& Other) noexcept;
        FScopedInputMode(const FScopedInputMode&) = delete;
        FScopedInputMode& operator=(const FScopedInputMode&) = delete;

        void Reset();

        bool IsHeld() const { return Id != 0; }

    private:

        TWeakObjectPtr<CWorld> World;
        uint64                 Id = 0;
    };
}
