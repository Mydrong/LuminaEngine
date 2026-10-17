#pragma once

#include "Containers/String.h"
#include "Core/Object/ObjectMacros.h"
#include "Events/KeyCodes.h"
#include "Events/MouseCodes.h"
#include "Input/Gamepad.h"
#include "Key.generated.h"

namespace Lumina
{
    // Which input device an SKey refers to.
    REFLECT()
    enum class EKeyDevice : uint8
    {
        None,
        Keyboard,
        Mouse,
        Gamepad,
    };

    // One bindable button on any device, with an optional Ctrl/Shift/Alt chord for keyboard and mouse.
    REFLECT()
    struct RUNTIME_API SKey
    {
        GENERATED_BODY()

        SKey() = default;
        explicit SKey(EKey InKey)        : Device(EKeyDevice::Keyboard), Key(InKey) {}
        explicit SKey(EMouseKey InMouse) : Device(EKeyDevice::Mouse), MouseButton(InMouse) {}
        explicit SKey(EGamepadButton InButton) : Device(EKeyDevice::Gamepad), GamepadButton(InButton) {}

        // A keyboard chord for hotkey defaults, such as SKey(EKey::S, true) for Ctrl+S.
        SKey(EKey InKey, bool bInCtrl, bool bInShift = false, bool bInAlt = false)
            : Device(EKeyDevice::Keyboard), Key(InKey), bShift(bInShift), bCtrl(bInCtrl), bAlt(bInAlt) {}

        // The device this binding targets. None == unbound.
        PROPERTY(Hidden)
        EKeyDevice Device = EKeyDevice::None;

        // The keyboard key, valid when Device == Keyboard.
        PROPERTY(Hidden)
        EKey Key = EKey::Num;

        // The mouse button, valid when Device == Mouse.
        PROPERTY(Hidden)
        EMouseKey MouseButton = EMouseKey::Num;

        // The gamepad button, valid when Device == Gamepad.
        PROPERTY(Hidden)
        EGamepadButton GamepadButton = EGamepadButton::Num;

        // Modifier chord, keyboard and mouse only. A binding to a bare modifier key leaves these false.
        PROPERTY(Hidden)
        bool bShift = false;
        PROPERTY(Hidden)
        bool bCtrl = false;
        PROPERTY(Hidden)
        bool bAlt = false;

        bool IsKeyboard() const { return Device == EKeyDevice::Keyboard; }
        bool IsMouse() const    { return Device == EKeyDevice::Mouse; }
        bool IsGamepad() const  { return Device == EKeyDevice::Gamepad; }

        bool IsValid() const
        {
            switch (Device)
            {
            case EKeyDevice::Keyboard: return Key != EKey::Num;
            case EKeyDevice::Mouse:    return MouseButton != EMouseKey::Num;
            case EKeyDevice::Gamepad:  return GamepadButton != EGamepadButton::Num;
            case EKeyDevice::None:     break;
            }
            return false;
        }

        void Clear() { *this = SKey(); }

        // Each setter rebinds to one device and drops the modifiers, which the caller adds back for a chord.
        void SetKey(EKey InKey)                         { *this = SKey(InKey); }
        void SetMouseButton(EMouseKey InButton)         { *this = SKey(InButton); }
        void SetGamepadButton(EGamepadButton InButton)  { *this = SKey(InButton); }

        // Such as "Ctrl+S", "F5", "Left Mouse Button", "Gamepad A" or "Gamepad Cross", or "None".
        FString GetDisplayName(EGamepadFamily Family = EGamepadFamily::Xbox) const;

        bool operator==(const SKey& Other) const
        {
            return Device == Other.Device && Key == Other.Key && MouseButton == Other.MouseButton && GamepadButton == Other.GamepadButton
                && bShift == Other.bShift && bCtrl == Other.bCtrl && bAlt == Other.bAlt;
        }
        bool operator!=(const SKey& Other) const { return !(*this == Other); }
    };

    // Stable names for settings files and UI, such as "LeftBracket" or "F5", and empty for a key without one.
    RUNTIME_API FString GetKeyDisplayName(EKey Key);
    RUNTIME_API FString GetMouseButtonDisplayName(EMouseKey Button);
}
