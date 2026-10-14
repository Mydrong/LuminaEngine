#pragma once

#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Object/SoftObjectPtr.h"
#include "Core/Math/Transform.h"
#include "Scripting/ScriptCallback.h"
#include "World/ECS/Entity.h"
#include "PrefabLibrary.generated.h"

namespace Lumina
{
    class CPrefab;
    class CWorld;

    /** Resolving a prefab asset and instantiating it, which is the prefab's business and not the world's. */
    REFLECT()
    class RUNTIME_API CPrefabLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** Instantiates at the origin, unparented. */
        FUNCTION()
        static ECS::FEntity SpawnPrefab(CWorld* World, const TSoftObjectPtr<CPrefab>& Prefab);

        /** Positions the spawned root at SpawnTransform, under Parent when it is not the null entity. */
        FUNCTION()
        static ECS::FEntity SpawnPrefabAt(CWorld* World, const TSoftObjectPtr<CPrefab>& Prefab,
            const FTransform& SpawnTransform, ECS::FEntity Parent);

        /** Loads the prefab off the game thread, then spawns and reports it once, back on the game thread. */
        FUNCTION()
        static void SpawnPrefabAsync(CWorld* World, const TSoftObjectPtr<CPrefab>& Prefab, SScriptCallback OnSpawned);
    };
}
