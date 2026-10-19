#include "RuntimePCH.h"
#include "RenderScene.h"

#include "World/World.h"

namespace Lumina
{
    void IRenderScene::RefreshFrameSettings()
    {
        const SWorldRenderSettings* Settings = World->FindRenderSettings();
        FrameSettings = Settings != nullptr ? *Settings : SWorldRenderSettings{};
    }
}
