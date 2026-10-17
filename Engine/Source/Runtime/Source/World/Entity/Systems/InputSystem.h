#pragma once
#include "EntitySystem.h"
#include "InputSystem.generated.h"

namespace Lumina
{
    REFLECT()
    class SInputSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;
        

        void OnUpdate(const SSystemContext& Context) override;
    };
}
