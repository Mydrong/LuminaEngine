#pragma once

#include "Assets/Factories/Factory.h"
#include "Assets/AssetTypes/Weather/WeatherPreset.h"
#include "WeatherFactories.generated.h"

namespace Lumina
{
    REFLECT()
    class CWeatherPresetFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;

        FString GetAssetName() const override { return "Weather Preset"; }
        FStringView GetDefaultAssetCreationName() override { return "NewWeatherPreset"; }
        FString GetAssetDescription() const override { return "A weather type built from rain, cloud and wind layers, played by a Weather component."; }
        CClass* GetAssetClass() const override { return CWeatherPreset::StaticClass(); }
        FString GetCategory() const override { return "Environment"; }
    };

    REFLECT()
    class CRainWeatherLayerFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;

        FString GetAssetName() const override { return "Rain Weather Layer"; }
        FStringView GetDefaultAssetCreationName() override { return "NewRainLayer"; }
        FString GetAssetDescription() const override { return "Falling rain, splashes, and the wetness, puddles and ripples it leaves on surfaces."; }
        CClass* GetAssetClass() const override { return CRainWeatherLayer::StaticClass(); }
        FString GetCategory() const override { return "Environment"; }
    };

    REFLECT()
    class CCloudWeatherLayerFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;

        FString GetAssetName() const override { return "Cloud Weather Layer"; }
        FStringView GetDefaultAssetCreationName() override { return "NewCloudLayer"; }
        FString GetAssetDescription() const override { return "Cloud coverage, thickness and storm lighting for a weather preset."; }
        CClass* GetAssetClass() const override { return CCloudWeatherLayer::StaticClass(); }
        FString GetCategory() const override { return "Environment"; }
    };

    REFLECT()
    class CWindWeatherLayerFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;

        FString GetAssetName() const override { return "Wind Weather Layer"; }
        FStringView GetDefaultAssetCreationName() override { return "NewWindLayer"; }
        FString GetAssetDescription() const override { return "Gusting wind that tilts the rain and drifts the clouds."; }
        CClass* GetAssetClass() const override { return CWindWeatherLayer::StaticClass(); }
        FString GetCategory() const override { return "Environment"; }
    };
}
