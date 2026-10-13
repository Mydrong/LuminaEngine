#pragma once

#include "CategorizedPicker.h"

namespace Lumina::EntityScriptPicker
{
    // Every class an entity can take as a script, C# and C++ alike, filed by the folder its source sits in. Keyed by class name.
    EDITOR_API void GatherItems(TVector<FPickerItem>& Out);

    // The category C++ scripts file under, which a picker lists after the project's own folders.
    inline constexpr const char* NativeCategory = "C++";
}
