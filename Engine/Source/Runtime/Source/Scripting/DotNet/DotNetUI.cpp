#include "DotNetUI.h"

#include "DotNetHost.h"
#include "World/World.h"

namespace Lumina::DotNetUI
{
    namespace
    {
        DotNet::TManagedExport<void (*)(uint64)> GPollModels("PollUIDataModels");
    }

    void PollModels(CWorld* World)
    {
        if (World == nullptr)
        {
            return;
        }
        if (auto* Fn = GPollModels.Get())
        {
            Fn(reinterpret_cast<uint64>(World));
        }
    }
}
