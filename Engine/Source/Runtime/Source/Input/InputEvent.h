#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Input/Key.h"
#include "InputEvent.generated.h"

namespace Lumina
{
    // The kind of discrete input that occurred, collapsed to the categories a gameplay script cares about.
    REFLECT()
    enum class EInputEventType : uint8
    {
        KeyDown,      // a keyboard key went down (Repeat flag set for OS auto-repeat)
        KeyUp,        // a keyboard key was released
        MouseDown,    // a mouse button went down
        MouseUp,      // a mouse button was released
        MouseMove,    // the cursor moved (DeltaX/DeltaY carry the motion)
        MouseScroll,  // the wheel turned (Scroll carries the signed delta)
        GamepadDown,  // a gamepad button went down, including a trigger or stick pushed past its threshold
        GamepadUp,    // a gamepad button was released
    };

    // Bit flags rather than bools, because a C# bool is not blittable and a ScriptEvent parameter must be.
    REFLECT()
    enum class EInputEventFlags : uint8
    {
        None   = 0,
        Shift  = 1 << 0,
        Ctrl   = 1 << 1,
        Alt    = 1 << 2,
        Repeat = 1 << 3,
    };

    // Flattened instead of carrying an SKey: SKey holds bools, and one non-blittable member makes the whole struct illegal as a ScriptEvent parameter, which is what kept OnInput dead for C# scripts.
    REFLECT(Event)
    struct SInputEvent
    {
        GENERATED_BODY()

        PROPERTY(Hidden)
        EInputEventType Type = EInputEventType::KeyDown;

        /** Which device Key/Button refers to. None for MouseMove and MouseScroll. */
        PROPERTY(Hidden)
        EKeyDevice Device = EKeyDevice::None;

        /** The keyboard key, valid when Device is Keyboard. */
        PROPERTY(Hidden)
        EKey Key = EKey::Num;

        /** The mouse button, valid when Device is Mouse. */
        PROPERTY(Hidden)
        EMouseKey Button = EMouseKey::Num;

        /** The gamepad button, valid when Device is Gamepad. */
        PROPERTY(Hidden)
        EGamepadButton GamepadButton = EGamepadButton::Num;

        PROPERTY(Hidden)
        EInputEventFlags Flags = EInputEventFlags::None;

        PROPERTY(Hidden)
        double MouseX = 0.0;

        PROPERTY(Hidden)
        double MouseY = 0.0;

        PROPERTY(Hidden)
        double DeltaX = 0.0;

        PROPERTY(Hidden)
        double DeltaY = 0.0;

        PROPERTY(Hidden)
        double Scroll = 0.0;

        bool HasFlag(EInputEventFlags Flag) const
        {
            return ((uint8)Flags & (uint8)Flag) != 0;
        }

        void AddFlag(EInputEventFlags Flag)
        {
            Flags = (EInputEventFlags)((uint8)Flags | (uint8)Flag);
        }

        bool IsShiftDown() const { return HasFlag(EInputEventFlags::Shift); }
        bool IsCtrlDown()  const { return HasFlag(EInputEventFlags::Ctrl); }
        bool IsAltDown()   const { return HasFlag(EInputEventFlags::Alt); }
        bool IsRepeat()    const { return HasFlag(EInputEventFlags::Repeat); }
        bool IsKeyboard()  const { return Device == EKeyDevice::Keyboard; }
        bool IsMouse()     const { return Device == EKeyDevice::Mouse; }
        bool IsGamepad()   const { return Device == EKeyDevice::Gamepad; }

        /** Rebuild the equivalent SKey, so chord matching against a bound SKey still works. */
        SKey ToKey() const
        {
            SKey Out;
            switch (Device)
            {
            case EKeyDevice::Keyboard: Out.SetKey(Key); break;
            case EKeyDevice::Mouse:    Out.SetMouseButton(Button); break;
            case EKeyDevice::Gamepad:  Out.SetGamepadButton(GamepadButton); return Out;
            case EKeyDevice::None:     break;
            }
            Out.bShift = IsShiftDown();
            Out.bCtrl  = IsCtrlDown();
            Out.bAlt   = IsAltDown();
            return Out;
        }

        void SetKey(const SKey& InKey)
        {
            Device        = InKey.Device;
            Key           = InKey.Key;
            Button        = InKey.MouseButton;
            GamepadButton = InKey.GamepadButton;
            if (InKey.bShift) { AddFlag(EInputEventFlags::Shift); }
            if (InKey.bCtrl)  { AddFlag(EInputEventFlags::Ctrl); }
            if (InKey.bAlt)   { AddFlag(EInputEventFlags::Alt); }
        }
    };
}
