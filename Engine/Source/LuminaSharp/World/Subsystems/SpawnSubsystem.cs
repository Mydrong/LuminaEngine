using System;
using LuminaSharp;

namespace Lumina;

// Placement overloads over the native spawns, so a caller passes a location rather than building a transform.
public unsafe partial class CSpawnSubsystem
{
    // Runs OnSpawned once on the game thread with the spawned root, or the null entity if it failed.
    public void SpawnPrefabAsync(TSoftObjectPtr<CPrefab> Prefab, Action<FEntity> OnSpawned)
        => SpawnPrefabAsync(Prefab, ScriptCallback.Of(OnSpawned));

    public FEntity SpawnPrefab(TSoftObjectPtr<CPrefab> Prefab, FVector3 Location, FQuat? Rotation = null, FEntity? Parent = null)
        => SpawnPrefabAt(Prefab, new FTransform(Location, Rotation ?? FQuat.Identity, FVector3.One), Parent ?? FEntity.Null);

    // A full transform, so an authored scale survives the spawn.
    public FEntity SpawnPrefab(TSoftObjectPtr<CPrefab> Prefab, FTransform Transform, FEntity? Parent = null)
        => SpawnPrefabAt(Prefab, Transform, Parent ?? FEntity.Null);

    // A prefab already loaded, with no path lookup. Null entity if the prefab is null.
    public FEntity SpawnPrefab(CPrefab Prefab) => SpawnLoadedPrefab(Prefab);

    public FEntity SpawnPrefab(CPrefab Prefab, FVector3 Location, FQuat? Rotation = null, FEntity? Parent = null)
        => SpawnLoadedPrefabAt(Prefab, new FTransform(Location, Rotation ?? FQuat.Identity, FVector3.One), Parent ?? FEntity.Null);

    public FEntity SpawnPrefab(CPrefab Prefab, FTransform Transform, FEntity? Parent = null)
        => SpawnLoadedPrefabAt(Prefab, Transform, Parent ?? FEntity.Null);
}
