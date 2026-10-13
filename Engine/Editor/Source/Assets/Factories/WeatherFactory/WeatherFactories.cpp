#include "EditorPCH.h"
#include "WeatherFactories.h"

namespace Lumina
{
    CObject* CWeatherPresetFactory::CreateNew(const FName& Name, CPackage* Package)
    {
        return NewObject<CWeatherPreset>(Package, Name);
    }

    CObject* CRainWeatherLayerFactory::CreateNew(const FName& Name, CPackage* Package)
    {
        return NewObject<CRainWeatherLayer>(Package, Name);
    }

    CObject* CCloudWeatherLayerFactory::CreateNew(const FName& Name, CPackage* Package)
    {
        return NewObject<CCloudWeatherLayer>(Package, Name);
    }

    CObject* CWindWeatherLayerFactory::CreateNew(const FName& Name, CPackage* Package)
    {
        return NewObject<CWindWeatherLayer>(Package, Name);
    }
}
