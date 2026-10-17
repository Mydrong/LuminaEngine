#pragma once

#include "Containers/Name.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "Events/KeyCodes.h"
#include "Events/MouseCodes.h"
#include "Input/InputAction.h"
#include "Input/InputMode.h"
#include "InputLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    /** Polling input for a world. Everything returns the neutral value when the world is not receiving input. */
    REFLECT()
    class RUNTIME_API CInputLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** False when the world has no viewport, is not the active one, or the editor holds input focus. */
        FUNCTION()
        static bool IsReceivingInput(CWorld* World);

        //~ Authored actions, the rebindable surface.

        /** This frame's evaluated state, zeroed when the name is not an authored action. */
        FUNCTION()
        static SInputActionState GetActionState(CWorld* World, const FName& Action);

        FUNCTION()
        static bool IsActionDown(CWorld* World, const FName& Action);

        FUNCTION()
        static bool WasActionPressed(CWorld* World, const FName& Action);

        FUNCTION()
        static bool WasActionReleased(CWorld* World, const FName& Action);

        FUNCTION()
        static bool IsActionHeld(CWorld* World, const FName& Action);

        FUNCTION()
        static bool WasActionTapped(CWorld* World, const FName& Action);

        FUNCTION()
        static bool WasActionDoubleTapped(CWorld* World, const FName& Action);

        /** True on the press and again each time a repeat comes due while held. */
        FUNCTION()
        static bool IsActionRepeated(CWorld* World, const FName& Action);

        FUNCTION()
        static bool IsActionBuffered(CWorld* World, const FName& Action);

        /** Takes the buffered press so it acts once, returning whether there was one. */
        FUNCTION()
        static bool ConsumeBufferedPress(CWorld* World, const FName& Action);

        FUNCTION()
        static float GetActionAxis(CWorld* World, const FName& Action);

        FUNCTION()
        static FVector2 GetActionAxis2D(CWorld* World, const FName& Action);

        FUNCTION()
        static float GetActionHeldTime(CWorld* World, const FName& Action);

        /** Plus one while Positive is down and minus one while Negative is, so holding both cancels. */
        FUNCTION()
        static float GetAxisPair(CWorld* World, const FName& Positive, const FName& Negative);

        /** Row of the action in the per-frame state table, or Constants::kIndexNone. Resolve once per settings serial. */
        FUNCTION()
        static int32 FindActionIndex(const FName& Action);

        //~ Mapping layers, per world. A blocking layer stops any action it does not list.

        FUNCTION()
        static void PushLayer(CWorld* World, const FName& Layer);

        /** False when the layer was not on the stack. */
        FUNCTION()
        static bool PopLayer(CWorld* World, const FName& Layer);

        FUNCTION()
        static bool HasLayer(CWorld* World, const FName& Layer);

        FUNCTION()
        static void ClearLayers(CWorld* World);

        //~ Raw device state, shared by every entity in the world and not rebindable.

        FUNCTION()
        static bool IsKeyDown(CWorld* World, EKey Key);

        FUNCTION()
        static bool WasKeyPressed(CWorld* World, EKey Key);

        FUNCTION()
        static bool WasKeyReleased(CWorld* World, EKey Key);

        FUNCTION()
        static bool IsMouseButtonDown(CWorld* World, EMouseKey Button);

        FUNCTION()
        static bool WasMouseButtonPressed(CWorld* World, EMouseKey Button);

        FUNCTION()
        static bool WasMouseButtonReleased(CWorld* World, EMouseKey Button);

        FUNCTION()
        static FVector2 GetMousePosition(CWorld* World);

        FUNCTION()
        static FVector2 GetMouseDelta(CWorld* World);

        FUNCTION()
        static float GetMouseWheel(CWorld* World);

        /** False when no pad is connected, or the world is not receiving input. */
        FUNCTION()
        static bool IsGamepadConnected(CWorld* World);

        FUNCTION()
        static bool IsGamepadButtonDown(CWorld* World, EGamepadButton Button);

        FUNCTION()
        static bool WasGamepadButtonPressed(CWorld* World, EGamepadButton Button);

        FUNCTION()
        static bool WasGamepadButtonReleased(CWorld* World, EGamepadButton Button);

        /** Dead-zoned, with stick Y positive up and triggers from 0 to 1. */
        FUNCTION()
        static float GetGamepadAxis(CWorld* World, EGamepadAxis Axis);

        FUNCTION()
        static FVector2 GetGamepadLeftStick(CWorld* World);

        FUNCTION()
        static FVector2 GetGamepadRightStick(CWorld* World);

        //~ Prompts, which follow whichever device the player touched last.

        FUNCTION()
        static EInputDevice GetLastUsedDevice(CWorld* World);

        FUNCTION()
        static EGamepadFamily GetGamepadFamily();

        /** Such as "E" or "Cross", and empty when the action has no binding on the last used device. */
        FUNCTION()
        static FString GetActionPrompt(CWorld* World, const FName& Action);

        //~ Rebinding, for a controls menu. A rebind applies at once and SaveSettings keeps it.

        FUNCTION()
        static int32 GetBindingCount(const FName& Action);

        FUNCTION()
        static FString GetBindingDisplayName(const FName& Action, int32 BindingIndex);

        /** Only key bindings rebind, so a continuous source such as Mouse X reports false. */
        FUNCTION()
        static bool IsBindingRebindable(const FName& Action, int32 BindingIndex);

        /** Captures the next key, mouse button or pad button pressed, where Escape cancels. */
        FUNCTION()
        static bool BeginRebind(const FName& Action, int32 BindingIndex);

        FUNCTION()
        static void CancelRebind();

        FUNCTION()
        static bool IsRebinding();

        /** Another action already bound to the key the last rebind captured, or none. */
        FUNCTION()
        static FName GetRebindConflict();

        /** Back to the authored keys for Action, or for every action when it is none. */
        FUNCTION()
        static void ResetBindings(const FName& Action);

        //~ Cursor and routing, which act on the world's own viewport rather than the receiving one.

        FUNCTION()
        static void SetInputMode(CWorld* World, EInputMode Mode);

        FUNCTION()
        static EInputMode GetInputMode(CWorld* World);

        FUNCTION()
        static void SetMouseMode(CWorld* World, EMouseMode Mode);

        FUNCTION()
        static EMouseMode GetMouseMode(CWorld* World);
    };
}
