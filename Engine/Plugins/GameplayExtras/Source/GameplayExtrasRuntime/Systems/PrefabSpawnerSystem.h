#pragma once

#include "World/Entity/Systems/EntitySystem.h"
#include "PrefabSpawnerSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class SPrefabSpawnerSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;
        

        void OnStartup(const SSystemContext& Context) override;
        void OnUpdate(const SSystemContext& Context) override;
        void OnTeardown(const SSystemContext& Context) override;
        
    };
}
