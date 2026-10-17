#pragma once
#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "SpriteAnimationSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class SSpriteAnimationSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnUpdate(const SSystemContext& Context) override;
    };
}
