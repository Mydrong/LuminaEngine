#pragma once

#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "FoliageTerrainSystem.generated.h"

namespace Lumina
{
    // Keeps painted foliage glued to the terrain surface: when a terrain's heightmap changes (sculpt, import),
    // re-projects the height of every follow-enabled foliage instance over the edited region. Runs in the
    // editor (Paused) and at runtime (FrameStart) so sculpting moves foliage live, with no repaint.
    REFLECT()
    class RUNTIME_API SFoliageTerrainSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnStartup(const SSystemContext& Context) override;
        void OnUpdate(const SSystemContext& Context) override;
    };
}
