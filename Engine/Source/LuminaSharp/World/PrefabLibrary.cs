using System;
using LuminaSharp;

namespace Lumina;

// Handwritten ergonomics over the reflected CPrefabLibrary.
public unsafe partial class CPrefabLibrary
{
    // Runs OnSpawned once on the game thread with the spawned root, or the null entity if it failed.
    public static void SpawnPrefabAsync(CWorld World, TSoftObjectPtr<CPrefab> Prefab, Action<FEntity> OnSpawned)
    {
        SpawnPrefabAsync(World, Prefab, ScriptCallback.Of(OnSpawned));
    }
}
