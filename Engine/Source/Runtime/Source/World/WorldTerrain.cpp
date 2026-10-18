#include "RuntimePCH.h"

#include "World/Subsystems/TerrainSculptSystem.h"
#include "World/Entity/Components/TerrainComponent.h"
#include "World/Entity/Components/TransformComponent.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // The first terrain whose footprint covers the point, with the height there.
        template<typename TVisit>
        bool VisitTerrainAt(CWorld& World, float X, float Z, TVisit&& Visit)
        {

            ECS::FRegistry& Registry = World.GetEntityRegistry();
            auto View = Registry.View<STerrainComponent, STransformComponent>();
            for (auto [Entity, Terrain, Transform] : View.Each())
            {
                const FVector3 Origin = Transform.GetWorldLocationCached();
                float Height = 0.0f;
                if (FTerrainSculptSystem::SampleHeight(Terrain, Origin, X, Z, Height))
                {
                    Visit(Terrain, Origin, Height);
                    return true;
                }
            }

            return false;
        }
    }

    bool CWorld::IsOnTerrain(float X, float Z)
    {
        return VisitTerrainAt(*this, X, Z, [](const STerrainComponent&, const FVector3&, float) {});
    }

    float CWorld::GetTerrainHeight(float X, float Z, float Fallback)
    {
        float Result = Fallback;
        VisitTerrainAt(*this, X, Z, [&](const STerrainComponent&, const FVector3&, float Height) { Result = Height; });
        return Result;
    }

    FVector3 CWorld::GetTerrainNormal(float X, float Z)
    {
        FVector3 Result(0.0f, 1.0f, 0.0f);
        VisitTerrainAt(*this, X, Z, [&](const STerrainComponent& Terrain, const FVector3& Origin, float)
        {
            Result = FTerrainSculptSystem::SampleNormal(Terrain, Origin, X, Z);
        });
        return Result;
    }
}
