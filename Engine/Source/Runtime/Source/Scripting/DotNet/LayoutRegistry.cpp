#include "RuntimePCH.h"

#include "LayoutRegistry.h"

#include <unordered_map>
#include <string>

namespace Lumina::DotNet
{
    namespace
    {
        // A function-local static, so the map exists before any self-registration runs.
        std::unordered_map<std::string, int32>& Registry()
        {
            static std::unordered_map<std::string, int32> Map;
            return Map;
        }
    }

    void RegisterLayout(const char* Key, int32 Size)
    {
        Registry()[std::string(Key)] = Size;
    }

    int32 GetLayoutSize(const char* Name, int32 Len)
    {
        if (Name == nullptr || Len <= 0)
        {
            return -1;
        }
        const std::unordered_map<std::string, int32>& Map = Registry();
        auto It = Map.find(std::string(Name, (size_t)Len));
        return It != Map.end() ? It->second : -1;
    }
}
