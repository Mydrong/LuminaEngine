#pragma once

#include "Containers/String.h"

namespace Lumina
{
    class CClass;
    class CObject;
}

namespace Lumina::AssetExport
{
    // Decided from the class alone, so a menu can offer export without loading the asset.
    EDITOR_API bool CanExport(const CClass* AssetClass);

    // Writes the file Asset was imported from into Directory, under the asset's name and a free file name.
    EDITOR_API bool Export(CObject* Asset, FStringView Directory, FString& OutPath, FString& OutError);
}
