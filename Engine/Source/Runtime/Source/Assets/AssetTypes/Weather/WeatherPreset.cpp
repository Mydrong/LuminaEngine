#include "RuntimePCH.h"
#include "WeatherPreset.h"

namespace Lumina
{
    FWeatherParams CWeatherPreset::Resolve() const
    {
        FWeatherParams Params;
        for (const TStrongObjectPtr<CWeatherLayer>& Layer : Layers)
        {
            // A layer still streaming in holds its class defaults, which would flash before its saved values land.
            if (Layer.IsValid() && !Layer->HasAnyFlag(OF_NeedsLoad))
            {
                Layer->ApplyTo(Params);
            }
        }
        return Params;
    }
}
