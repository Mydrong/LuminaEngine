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
    // Input settings shared by every world, such as rebinding, while polling a world's input lives on CWorld.
    REFLECT()
    class RUNTIME_API CInputLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** Row of the action in the per-frame state table, or Constants::kIndexNone. Resolve once per settings serial. */
        FUNCTION()
        static int32 FindActionIndex(const FName& Action);

        FUNCTION()
        static EGamepadFamily GetGamepadFamily();

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
    };
}
