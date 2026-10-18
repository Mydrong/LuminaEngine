#pragma once

#include "Containers/Vector.h"
#include "Core/LuminaMacros.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Events/MouseCodes.h"
#include "Input/InputMode.h"

namespace Lumina
{
    class CWorld;

    // The input and mouse modes every open panel asked for, combined so each holder releases only its own request.
    class RUNTIME_API FInputModeLeases
    {
    public:

        explicit FInputModeLeases(CWorld* InWorld) : World(InWorld) {}
        LE_NO_COPYMOVE(FInputModeLeases);

        // The modes from before the first lease come back when the last one is released.
        uint64 Acquire(EInputMode InputMode, EMouseMode MouseMode);
        bool   Release(uint64 Id);

        // Drops every lease and restores the earlier modes, for a world going away.
        void Clear();

        int32 Num() const { return (int32)Leases.size(); }

    private:

        struct FLease
        {
            uint64     Id;
            EInputMode InputMode;
            EMouseMode MouseMode;
        };

        void Apply();

        CWorld*        World;
        TVector<FLease> Leases;
        uint64         NextId = 1;
        EInputMode     BaseInputMode = EInputMode::Game;
        EMouseMode     BaseMouseMode = EMouseMode::Captured;
    };

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
