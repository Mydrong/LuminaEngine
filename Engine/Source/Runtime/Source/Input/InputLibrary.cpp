#include "RuntimePCH.h"

#include "InputLibrary.h"

#include "Input/InputActionMap.h"
#include "Input/InputQuery.h"

namespace Lumina
{
    int32 CInputLibrary::FindActionIndex(const FName& Action)
    {
        return FInputActionMap::Get().FindActionIndex(Action);
    }

    EGamepadFamily CInputLibrary::GetGamepadFamily()
    {
        return Input::GetGamepadFamily();
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
}
