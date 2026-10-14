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
        return Input::IsActionDown(World, SInputActionHandle{ Action });
    }

    bool CInputLibrary::WasActionPressed(CWorld* World, const FName& Action)
    {
        return Input::IsActionPressed(World, SInputActionHandle{ Action });
    }

    bool CInputLibrary::WasActionReleased(CWorld* World, const FName& Action)
    {
        return Input::IsActionReleased(World, SInputActionHandle{ Action });
    }

    bool CInputLibrary::IsActionHeld(CWorld* World, const FName& Action)
    {
        return Input::IsActionHeld(World, SInputActionHandle{ Action });
    }

    bool CInputLibrary::WasActionTapped(CWorld* World, const FName& Action)
    {
        return Input::WasActionTapped(World, SInputActionHandle{ Action });
    }

    float CInputLibrary::GetActionAxis(CWorld* World, const FName& Action)
    {
        return Input::GetActionAxis(World, SInputActionHandle{ Action });
    }

    FVector2 CInputLibrary::GetActionAxis2D(CWorld* World, const FName& Action)
    {
        return Input::GetActionAxis2D(World, SInputActionHandle{ Action });
    }

    float CInputLibrary::GetActionHeldTime(CWorld* World, const FName& Action)
    {
        return Input::GetActionHeldTime(World, SInputActionHandle{ Action });
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
