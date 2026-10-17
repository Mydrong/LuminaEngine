#pragma once

#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "BuoyancySystem.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API SBuoyancySystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;
        

        void OnStartup(const SSystemContext& Context) override;
        void OnUpdate(const SSystemContext& Context) override;
    };
}
