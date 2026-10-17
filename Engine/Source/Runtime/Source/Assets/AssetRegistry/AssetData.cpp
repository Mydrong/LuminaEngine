#include "RuntimePCH.h"
#include "AssetData.h"

#include "Core/Object/Class.h"
#include "Core/Object/ObjectCore.h"

namespace Lumina
{
    FStringView LexToString(EDependencyType T)
    {
        switch (T)
        {
            case EDependencyType::Hard:       return "Hard";
            case EDependencyType::Soft:       return "Soft";
            case EDependencyType::Script:     return "Script";
            case EDependencyType::Owned:      return "Owned";
            case EDependencyType::EditorOnly: return "EditorOnly";
            case EDependencyType::Generated:  return "Generated";
            default:                          return "Unknown";
        }
    }

    bool FAssetData::IsClassMissing() const
    {
        // A record the scan has not read yet has no class at all, which is not the same as a removed one.
        return !AssetClass.IsNone() && FindObject<CClass>(AssetClass) == nullptr;
    }
}
