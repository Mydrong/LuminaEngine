#include "RuntimePCH.h"

#include "Assets/AssetManager/AssetManager.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/AssetTypes/Prefabs/Prefab.h"
#include "Core/Object/Cast.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Log/Log.h"
#include "World/World.h"

namespace Lumina
{
    ECS::FEntity CWorld::SpawnPrefab(const TSoftObjectPtr<CPrefab>& Prefab)
    {
        return SpawnPrefabAt(Prefab, FTransform(), ECS::NullEntity);
    }

    ECS::FEntity CWorld::SpawnPrefabAt(const TSoftObjectPtr<CPrefab>& Prefab,
        const FTransform& SpawnTransform, ECS::FEntity Parent)
    {
        FStringView Path = Prefab.GetPath();
        FAssetData* AssetData = FAssetRegistry::Get().GetAssetByPath(Path);
        if (AssetData == nullptr)
        {
            LOG_WARN("SpawnPrefab: no asset found at path '{}'", Path);
            return ECS::NullEntity;
        }

        // A cold spawn fans the prefab's closure across the swarm; a resident one takes the lookup.
        CPrefab* PrefabObject = FindObject<CPrefab>(AssetData->AssetGUID);
        if (PrefabObject == nullptr)
        {
            PrefabObject = LoadObjectGraph<CPrefab>(AssetData->AssetGUID);
        }
        if (PrefabObject == nullptr)
        {
            LOG_WARN("SpawnPrefab: asset '{}' is not a CPrefab", Path);
            return ECS::NullEntity;
        }

        return SpawnLoadedPrefabAt(PrefabObject, SpawnTransform, Parent);
    }

    ECS::FEntity CWorld::SpawnLoadedPrefab(CPrefab* Prefab)
    {
        return SpawnLoadedPrefabAt(Prefab, FTransform(), ECS::NullEntity);
    }

    ECS::FEntity CWorld::SpawnLoadedPrefabAt(CPrefab* Prefab, const FTransform& SpawnTransform, ECS::FEntity Parent)
    {
        if (Prefab == nullptr)
        {
            return ECS::NullEntity;
        }
        return Prefab->Instantiate(this, SpawnTransform, Parent);
    }

    void CWorld::SpawnPrefabAsync(const TSoftObjectPtr<CPrefab>& Prefab, SScriptCallback OnSpawned)
    {
        const FName Path(Prefab.GetPath());
        TStrongObjectPtr<CWorld> WorldHandle(this);
        FAssetManager::Get().Load(Prefab.GetPath()).OnReady([WorldHandle, Path, OnSpawned](const FAssetHandle& Handle)
        {
            CWorld* Target = WorldHandle.Get();
            CPrefab* Loaded = Handle.Get<CPrefab>();
            if (Target == nullptr || Loaded == nullptr)
            {
                if (Loaded == nullptr)
                {
                    LOG_WARN("SpawnPrefabAsync: asset '{}' is not a CPrefab", Path.c_str());
                }
                Scripting::InvokeScriptCallback(OnSpawned, (uint64)(uint32)ECS::NullEntity);
                return;
            }

            const ECS::FEntity Spawned = Target->SpawnLoadedPrefab(Loaded);
            Scripting::InvokeScriptCallback(OnSpawned, (uint64)(uint32)Spawned);
        });
    }
}
