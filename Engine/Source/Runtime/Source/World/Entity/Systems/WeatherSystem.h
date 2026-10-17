#pragma once

#include "EntitySystem.h"
#include "WeatherState.h"
#include "Core/Object/ObjectMacros.h"
#include "WeatherSystem.generated.h"

namespace Lumina
{
    class CWeatherPreset;

    REFLECT()
    class RUNTIME_API SWeatherSystem : public CEntitySystem
    {
        GENERATED_BODY()

    public:

        void Configure() override;
        void OnStartup(const SSystemContext& Context) override;
        void OnUpdate(const SSystemContext& Context) override;
        bool HasWork(const SSystemContext& Context) override;

    private:

        const CWeatherPreset* ActivePreset = nullptr;
        bool                  bHasActivePreset = false;
        FWeatherParams        TransitionFrom;
        float                 TransitionAlpha = 1.0f;
        double                GustTime = 0.0;
    };
}
