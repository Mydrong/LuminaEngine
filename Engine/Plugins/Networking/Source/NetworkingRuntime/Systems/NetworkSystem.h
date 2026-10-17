#pragma once
#include "World/Entity/Systems/EntitySystem.h"
#include "NetworkSystem.generated.h"

namespace Lumina
{
    // Per-world networking driver.
    REFLECT()
    class SNetworkSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;

        void OnUpdate(const SSystemContext& Context) override;
        bool HasWork(const SSystemContext& Context) override;
    };
}
