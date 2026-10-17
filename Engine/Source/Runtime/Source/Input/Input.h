#pragma once

#include "Core/Math/Math.h"
#include "Platform/GenericPlatform.h"

namespace Lumina::Input
{
    // One state machine for every button-like input, so keys, mouse buttons and gamepad buttons share their edge rules.
    enum class EButtonState : uint8
    {
        Up,
        Pressed,
        Held,
        Released,
    };

    inline bool IsDown(EButtonState State) { return State == EButtonState::Pressed || State == EButtonState::Held; }

    // Pressed becomes Held and Released becomes Up, the step every device takes at the end of a frame.
    inline EButtonState Settle(EButtonState State)
    {
        if (State == EButtonState::Pressed)  { return EButtonState::Held; }
        if (State == EButtonState::Released) { return EButtonState::Up; }
        return State;
    }

    // The next state of a button that was State and is now bDown, for devices that are polled rather than evented.
    inline EButtonState Advance(EButtonState State, bool bDown)
    {
        if (bDown)
        {
            return IsDown(State) ? EButtonState::Held : EButtonState::Pressed;
        }
        return IsDown(State) ? EButtonState::Released : EButtonState::Up;
    }

    // Zero inside the dead zone, with the rest rescaled so full deflection still reaches one.
    inline float ApplyDeadZone(float Value, float DeadZone)
    {
        const float Magnitude = Math::Abs(Value);
        if (DeadZone <= 0.0f)
        {
            return Value;
        }
        return Magnitude <= DeadZone ? 0.0f : Math::Sign(Value) * (Magnitude - DeadZone) / (1.0f - DeadZone);
    }

    // The two-axis form, measured on the combined length so a diagonal is not cut twice.
    inline void ApplyRadialDeadZone(float& X, float& Y, float DeadZone)
    {
        if (DeadZone <= 0.0f)
        {
            return;
        }
        const float Magnitude = Math::Sqrt(X * X + Y * Y);
        if (Magnitude <= DeadZone)
        {
            X = 0.0f;
            Y = 0.0f;
            return;
        }
        const float Scale = (Magnitude - DeadZone) / ((1.0f - DeadZone) * Magnitude);
        X *= Scale;
        Y *= Scale;
    }
}
