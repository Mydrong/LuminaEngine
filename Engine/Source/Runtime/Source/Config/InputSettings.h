#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Config/DeveloperSettings.h"
#include "Containers/Vector.h"
#include "Input/InputAction.h"
#include "InputSettings.generated.h"

namespace Lumina
{
    // The project's actions, layers and gamepad tuning, edited under Engine > Input and saved to /Config/InputSettings.json.
    REFLECT(MinimalAPI, ConfigFile = "/Config/InputSettings.json", DisplayName = "Input", Category = "Engine")
    class CInputSettings : public CDeveloperSettings
    {
        GENERATED_BODY()
    public:

        /** Named gameplay inputs and the keys/buttons that fire them. */
        PROPERTY(Editable, Category = "Action Mappings", TitleProperty = "Name")
        TVector<SInputAction> Actions;

        /** Named layers a world can push to change what input means, e.g. a Menu layer that blocks gameplay. */
        PROPERTY(Editable, Category = "Mapping Contexts", TitleProperty = "Name")
        TVector<SInputMappingContext> MappingContexts;

        /** Stick travel ignored around center, which hides drift on worn sticks. The rest is rescaled to full range. */
        PROPERTY(Editable, Category = "Gamepad", ClampMin = 0.0f, ClampMax = 0.9f)
        float GamepadStickDeadZone = 0.15f;

        /** Trigger travel ignored at rest. */
        PROPERTY(Editable, Category = "Gamepad", ClampMin = 0.0f, ClampMax = 0.9f)
        float GamepadTriggerDeadZone = 0.05f;

        /** How far a trigger must be pulled to count as a pressed button. */
        PROPERTY(Editable, Category = "Gamepad", ClampMin = 0.05f, ClampMax = 1.0f)
        float GamepadTriggerPressThreshold = 0.3f;

        /** How far a stick must be pushed to count as a pressed direction, such as Left Stick Up in a menu. */
        PROPERTY(Editable, Category = "Gamepad", ClampMin = 0.05f, ClampMax = 1.0f)
        float GamepadStickPressThreshold = 0.5f;

        // Push the loaded actions into the live FInputActionMap.
        virtual void PostInitSettings() override;
    };
}
