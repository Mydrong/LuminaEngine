#include "RuntimePCH.h"

#include "InputLibrary.h"

#include "Input/InputActionMap.h"
#include "Input/InputQuery.h"

namespace Lumina
{
    bool CInputLibrary::IsReceivingInput(CWorld* World)
    {
        return Input::IsReceivingInput(World);
    }

    SInputActionState CInputLibrary::GetActionState(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action });
    }

    bool CInputLibrary::IsActionDown(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsDown();
    }

    bool CInputLibrary::WasActionPressed(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsPressed();
    }

    bool CInputLibrary::WasActionReleased(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsReleased();
    }

    bool CInputLibrary::IsActionHeld(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsHeld();
    }

    bool CInputLibrary::WasActionTapped(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsTapped();
    }

    bool CInputLibrary::WasActionDoubleTapped(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsDoubleTapped();
    }

    bool CInputLibrary::IsActionRepeated(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsRepeated();
    }

    bool CInputLibrary::IsActionBuffered(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).IsBuffered();
    }

    bool CInputLibrary::ConsumeBufferedPress(CWorld* World, const FName& Action)
    {
        return Input::ConsumeBufferedPress(World, SInputActionHandle{ Action });
    }

    float CInputLibrary::GetActionAxis(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).X;
    }

    FVector2 CInputLibrary::GetActionAxis2D(CWorld* World, const FName& Action)
    {
        const SInputActionState& State = Input::GetActionState(World, SInputActionHandle{ Action });
        return FVector2(State.X, State.Y);
    }

    float CInputLibrary::GetActionHeldTime(CWorld* World, const FName& Action)
    {
        return Input::GetActionState(World, SInputActionHandle{ Action }).HeldTime;
    }

    float CInputLibrary::GetAxisPair(CWorld* World, const FName& Positive, const FName& Negative)
    {
        return Input::GetAxisPair(World, SInputActionHandle{ Positive }, SInputActionHandle{ Negative });
    }

    int32 CInputLibrary::FindActionIndex(const FName& Action)
    {
        return FInputActionMap::Get().FindActionIndex(Action);
    }

    void CInputLibrary::PushLayer(CWorld* World, const FName& Layer)
    {
        Input::PushLayer(World, Layer);
    }

    bool CInputLibrary::PopLayer(CWorld* World, const FName& Layer)
    {
        return Input::PopLayer(World, Layer);
    }

    bool CInputLibrary::HasLayer(CWorld* World, const FName& Layer)
    {
        return Input::HasLayer(World, Layer);
    }

    void CInputLibrary::ClearLayers(CWorld* World)
    {
        Input::ClearLayers(World);
    }

    bool CInputLibrary::IsKeyDown(CWorld* World, EKey Key)
    {
        return Input::IsKeyDown(World, Key);
    }

    bool CInputLibrary::WasKeyPressed(CWorld* World, EKey Key)
    {
        return Input::IsKeyPressed(World, Key);
    }

    bool CInputLibrary::WasKeyReleased(CWorld* World, EKey Key)
    {
        return Input::IsKeyReleased(World, Key);
    }

    bool CInputLibrary::IsMouseButtonDown(CWorld* World, EMouseKey Button)
    {
        return Input::IsMouseButtonDown(World, Button);
    }

    bool CInputLibrary::WasMouseButtonPressed(CWorld* World, EMouseKey Button)
    {
        return Input::IsMouseButtonPressed(World, Button);
    }

    bool CInputLibrary::WasMouseButtonReleased(CWorld* World, EMouseKey Button)
    {
        return Input::IsMouseButtonReleased(World, Button);
    }

    FVector2 CInputLibrary::GetMousePosition(CWorld* World)
    {
        return Input::GetMousePosition(World);
    }

    FVector2 CInputLibrary::GetMouseDelta(CWorld* World)
    {
        return Input::GetMouseDelta(World);
    }

    float CInputLibrary::GetMouseWheel(CWorld* World)
    {
        return Input::GetMouseWheel(World);
    }

    bool CInputLibrary::IsGamepadConnected(CWorld* World)
    {
        return Input::IsGamepadConnected(World);
    }

    bool CInputLibrary::IsGamepadButtonDown(CWorld* World, EGamepadButton Button)
    {
        return Input::IsGamepadButtonDown(World, Button);
    }

    bool CInputLibrary::WasGamepadButtonPressed(CWorld* World, EGamepadButton Button)
    {
        return Input::IsGamepadButtonPressed(World, Button);
    }

    bool CInputLibrary::WasGamepadButtonReleased(CWorld* World, EGamepadButton Button)
    {
        return Input::IsGamepadButtonReleased(World, Button);
    }

    float CInputLibrary::GetGamepadAxis(CWorld* World, EGamepadAxis Axis)
    {
        return Input::GetGamepadAxis(World, Axis);
    }

    FVector2 CInputLibrary::GetGamepadLeftStick(CWorld* World)
    {
        return Input::GetGamepadLeftStick(World);
    }

    FVector2 CInputLibrary::GetGamepadRightStick(CWorld* World)
    {
        return Input::GetGamepadRightStick(World);
    }

    EInputDevice CInputLibrary::GetLastUsedDevice(CWorld* World)
    {
        return Input::GetLastUsedDevice(World);
    }

    EGamepadFamily CInputLibrary::GetGamepadFamily()
    {
        return Input::GetGamepadFamily();
    }

    FString CInputLibrary::GetActionPrompt(CWorld* World, const FName& Action)
    {
        return Input::GetActionPrompt(World, SInputActionHandle{ Action });
    }

    int32 CInputLibrary::GetBindingCount(const FName& Action)
    {
        return Input::GetBindingCount(Action);
    }

    FString CInputLibrary::GetBindingDisplayName(const FName& Action, int32 BindingIndex)
    {
        return Input::GetBindingDisplayName(Action, BindingIndex);
    }

    bool CInputLibrary::IsBindingRebindable(const FName& Action, int32 BindingIndex)
    {
        return Input::IsBindingRebindable(Action, BindingIndex);
    }

    bool CInputLibrary::BeginRebind(const FName& Action, int32 BindingIndex)
    {
        return Input::BeginRebind(Action, BindingIndex);
    }

    void CInputLibrary::CancelRebind()
    {
        Input::CancelRebind();
    }

    bool CInputLibrary::IsRebinding()
    {
        return Input::IsRebinding();
    }

    FName CInputLibrary::GetRebindConflict()
    {
        return Input::GetRebindConflict();
    }

    void CInputLibrary::ResetBindings(const FName& Action)
    {
        Input::ResetBindings(Action);
    }

    void CInputLibrary::SetInputMode(CWorld* World, EInputMode Mode)
    {
        Input::SetInputMode(World, Mode);
    }

    EInputMode CInputLibrary::GetInputMode(CWorld* World)
    {
        return Input::GetInputMode(World);
    }

    void CInputLibrary::SetMouseMode(CWorld* World, EMouseMode Mode)
    {
        Input::SetMouseMode(World, Mode);
    }

    EMouseMode CInputLibrary::GetMouseMode(CWorld* World)
    {
        return Input::GetMouseMode(World);
    }
}
