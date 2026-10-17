#pragma once

#include "Containers/StaticArray.h"
#include "Containers/String.h"
#include "Core/Object/ObjectMacros.h"
#include "Input/Input.h"
#include "Gamepad.generated.h"

namespace Lumina
{
    // Positional names in the standard gamepad layout, so one binding reads the same button on Xbox, PlayStation and Switch pads.
    REFLECT()
    enum class EGamepadButton : uint8
    {
        FaceBottom,
        FaceRight,
        FaceLeft,
        FaceTop,
        LeftShoulder,
        RightShoulder,
        Back,
        Start,
        Guide,
        LeftThumb,
        RightThumb,
        DPadUp,
        DPadRight,
        DPadDown,
        DPadLeft,

        // Analog inputs read as buttons past a threshold, so a menu or a digital action can bind them.
        LeftTrigger,
        RightTrigger,
        LeftStickUp,
        LeftStickDown,
        LeftStickLeft,
        LeftStickRight,
        RightStickUp,
        RightStickDown,
        RightStickLeft,
        RightStickRight,

        Num,
    };

    // Stick Y is positive up, unlike the raw device, and triggers run from 0 at rest to 1 fully pulled.
    REFLECT()
    enum class EGamepadAxis : uint8
    {
        LeftX,
        LeftY,
        RightX,
        RightY,
        LeftTrigger,
        RightTrigger,

        Num,
    };

    // Which button labels the connected pad prints, so prompts can match the controller in the player's hands.
    REFLECT()
    enum class EGamepadFamily : uint8
    {
        Generic,
        Xbox,
        PlayStation,
        Switch,
    };

    // The device the player touched last, which on-screen prompts follow.
    REFLECT()
    enum class EInputDevice : uint8
    {
        KeyboardMouse,
        Gamepad,
    };

    inline constexpr int32 kMaxGamepads = 4;

    // One pad's state for a frame, already dead-zoned, with button edges computed against the previous frame.
    struct FGamepadState
    {
        TArray<Input::EButtonState, (uint32)EGamepadButton::Num> Buttons = {};
        TArray<float, (uint32)EGamepadAxis::Num>                 Axes = {};
        EGamepadFamily                                           Family = EGamepadFamily::Generic;
        bool                                                     bConnected = false;

        Input::EButtonState GetButton(EGamepadButton Button) const { return Buttons[(uint32)Button]; }
        float               GetAxis(EGamepadAxis Axis) const       { return Axes[(uint32)Axis]; }

        bool IsButtonDown(EGamepadButton Button) const     { return Input::IsDown(GetButton(Button)); }
        bool IsButtonPressed(EGamepadButton Button) const  { return GetButton(Button) == Input::EButtonState::Pressed; }
        bool IsButtonReleased(EGamepadButton Button) const { return GetButton(Button) == Input::EButtonState::Released; }
    };

    // Thresholds the device layer shapes raw readings with, taken from CInputSettings each frame.
    struct FGamepadTuning
    {
        float StickDeadZone = 0.15f;
        float TriggerDeadZone = 0.05f;
        float TriggerPressThreshold = 0.3f;
        float StickPressThreshold = 0.5f;
    };

    // Every connected pad, polled once per frame. Owned by FInputViewportRegistry, which hands the result to the receiving context.
    class FGamepadDevices
    {
    public:

        RUNTIME_API void Poll(const FGamepadTuning& Tuning);

        // Releases everything, and a button still held stays ignored until it is let go, as a key held through a focus loss is.
        RUNTIME_API void Reset();

        // Every connected pad folded into one, with the family of the pad used most recently.
        RUNTIME_API const FGamepadState& GetCombined() const { return Combined; }

        // The first button that went down this frame on any pad, for a key binder listening for a press, or Num.
        RUNTIME_API EGamepadButton GetFirstPressedButton() const;

    private:

        TArray<FGamepadState, kMaxGamepads> Pads = {};
        TArray<FString, kMaxGamepads>       Names = {};
        TArray<TArray<bool, (uint32)EGamepadButton::Num>, kMaxGamepads> SuppressedUntilRelease = {};
        FGamepadState                       Combined;
        int32                               LastActivePad = Constants::kIndexNone;
    };

    // The label the pad prints, such as "A", "Cross" or "B" for the bottom face button on Xbox, PlayStation and Switch.
    RUNTIME_API FString GetGamepadButtonDisplayName(EGamepadButton Button, EGamepadFamily Family = EGamepadFamily::Xbox);
    RUNTIME_API FString GetGamepadAxisDisplayName(EGamepadAxis Axis, EGamepadFamily Family = EGamepadFamily::Xbox);
}
