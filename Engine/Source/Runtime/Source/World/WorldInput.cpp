#include "RuntimePCH.h"

#include "Input/InputActionMap.h"
#include "Input/InputQuery.h"
#include "World/World.h"

namespace Lumina
{
    bool CWorld::IsReceivingInput()
    {
        return Input::IsReceivingInput(this);
    }

    SInputActionState CWorld::GetActionState(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action });
    }

    bool CWorld::IsActionDown(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsDown();
    }

    bool CWorld::WasActionPressed(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsPressed();
    }

    bool CWorld::WasActionReleased(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsReleased();
    }

    bool CWorld::IsActionHeld(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsHeld();
    }

    bool CWorld::WasActionTapped(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsTapped();
    }

    bool CWorld::WasActionDoubleTapped(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsDoubleTapped();
    }

    bool CWorld::IsActionRepeated(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsRepeated();
    }

    bool CWorld::IsActionBuffered(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).IsBuffered();
    }

    bool CWorld::ConsumeBufferedPress(const FName& Action)
    {
        return Input::ConsumeBufferedPress(this, SInputActionHandle{ Action });
    }

    float CWorld::GetActionAxis(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).X;
    }

    FVector2 CWorld::GetActionAxis2D(const FName& Action)
    {
        const SInputActionState& State = Input::GetActionState(this, SInputActionHandle{ Action });
        return FVector2(State.X, State.Y);
    }

    float CWorld::GetActionHeldTime(const FName& Action)
    {
        return Input::GetActionState(this, SInputActionHandle{ Action }).HeldTime;
    }

    float CWorld::GetAxisPair(const FName& Positive, const FName& Negative)
    {
        return Input::GetAxisPair(this, SInputActionHandle{ Positive }, SInputActionHandle{ Negative });
    }

    void CWorld::PushLayer(const FName& Layer)
    {
        Input::PushLayer(this, Layer);
    }

    bool CWorld::PopLayer(const FName& Layer)
    {
        return Input::PopLayer(this, Layer);
    }

    bool CWorld::HasLayer(const FName& Layer)
    {
        return Input::HasLayer(this, Layer);
    }

    void CWorld::ClearLayers()
    {
        Input::ClearLayers(this);
    }

    bool CWorld::IsKeyDown(EKey Key)
    {
        return Input::IsKeyDown(this, Key);
    }

    bool CWorld::WasKeyPressed(EKey Key)
    {
        return Input::IsKeyPressed(this, Key);
    }

    bool CWorld::WasKeyReleased(EKey Key)
    {
        return Input::IsKeyReleased(this, Key);
    }

    bool CWorld::IsMouseButtonDown(EMouseKey Button)
    {
        return Input::IsMouseButtonDown(this, Button);
    }

    bool CWorld::WasMouseButtonPressed(EMouseKey Button)
    {
        return Input::IsMouseButtonPressed(this, Button);
    }

    bool CWorld::WasMouseButtonReleased(EMouseKey Button)
    {
        return Input::IsMouseButtonReleased(this, Button);
    }

    FVector2 CWorld::GetMousePosition()
    {
        return Input::GetMousePosition(this);
    }

    FVector2 CWorld::GetMouseDelta()
    {
        return Input::GetMouseDelta(this);
    }

    float CWorld::GetMouseWheel()
    {
        return Input::GetMouseWheel(this);
    }

    bool CWorld::IsGamepadConnected()
    {
        return Input::IsGamepadConnected(this);
    }

    bool CWorld::IsGamepadButtonDown(EGamepadButton Button)
    {
        return Input::IsGamepadButtonDown(this, Button);
    }

    bool CWorld::WasGamepadButtonPressed(EGamepadButton Button)
    {
        return Input::IsGamepadButtonPressed(this, Button);
    }

    bool CWorld::WasGamepadButtonReleased(EGamepadButton Button)
    {
        return Input::IsGamepadButtonReleased(this, Button);
    }

    float CWorld::GetGamepadAxis(EGamepadAxis Axis)
    {
        return Input::GetGamepadAxis(this, Axis);
    }

    FVector2 CWorld::GetGamepadLeftStick()
    {
        return Input::GetGamepadLeftStick(this);
    }

    FVector2 CWorld::GetGamepadRightStick()
    {
        return Input::GetGamepadRightStick(this);
    }

    EInputDevice CWorld::GetLastUsedDevice()
    {
        return Input::GetLastUsedDevice(this);
    }

    FString CWorld::GetActionPrompt(const FName& Action)
    {
        return Input::GetActionPrompt(this, SInputActionHandle{ Action });
    }

    void CWorld::SetInputMode(EInputMode Mode)
    {
        Input::SetInputMode(this, Mode);
    }

    EInputMode CWorld::GetInputMode()
    {
        return Input::GetInputMode(this);
    }

    void CWorld::SetMouseMode(EMouseMode Mode)
    {
        Input::SetMouseMode(this, Mode);
    }

    EMouseMode CWorld::GetMouseMode()
    {
        return Input::GetMouseMode(this);
    }

    uint64 CWorld::AcquireInputMode(EInputMode InputMode, EMouseMode MouseMode)
    {
        return GetInputModeLeases().Acquire(InputMode, MouseMode);
    }

    bool CWorld::ReleaseInputMode(uint64 Lease)
    {
        return Lease != 0 && GetInputModeLeases().Release(Lease);
    }
}
