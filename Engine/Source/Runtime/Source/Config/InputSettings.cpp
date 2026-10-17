#include "RuntimePCH.h"
#include "InputSettings.h"

#include "Input/InputActionMap.h"

namespace Lumina
{
    void CInputSettings::PostInitSettings()
    {
        // On the default object too, so the Input settings panel shows the migrated type.
        for (SInputAction& Action : Actions)
        {
            Action.FoldLegacyAxisFlag();
        }

        FInputActionMap::Get().RebuildFromSettings();
    }
}
