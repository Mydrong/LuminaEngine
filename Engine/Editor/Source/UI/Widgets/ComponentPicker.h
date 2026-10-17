#pragma once

#include "CategorizedPicker.h"

namespace Lumina
{
    class CStruct;
}

namespace Lumina::ComponentPicker
{
    // Every component an entity can take, keyed by struct name. C# ones file by source folder like scripts, C++ ones under C++ by the module that declares them.
    EDITOR_API void GatherItems(TVector<FPickerItem>& Out, const TFunction<bool(CStruct*)>& IsAlreadyPresent);

    // The component struct a picked item's key names, or null when it has gone, as after a script reload.
    EDITOR_API CStruct* FindComponent(FStringView Key);
}
