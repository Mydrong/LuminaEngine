#pragma once

#include "World/ECS/Registry.h"


#include "Containers/HashTable.h"
#include "Containers/Function.h"
#include "Containers/Name.h"
#include "Core/Math/Transform.h"

namespace Lumina
{
    class CWorld;
    class CObject;

    // Spawns/applies a dropped asset. Returns the affected entity (created or DropTarget); ECS::NullEntity
    // on failure.
    //
    // DropTarget is the entity the drop LANDED ON -- the outliner row, or whatever the viewport ray hit.
    // It is what a handler acts on (set this mesh's material, play this clip on that character), and it
    // says nothing about hierarchy.
    //
    // bAttachToTarget is the separate question of whether the gesture MEANT "make it a child of that".
    // Only an outliner row drop does; placing into the viewport does not, or dropping a crate onto the
    // floor would parent the crate to the floor. Handlers that spawn consult it before reparenting;
    // handlers that only modify DropTarget ignore it.
    using FEditorAssetDropHandler = TFunction<ECS::FEntity(CWorld* World, CObject* Asset, const FTransform& SpawnTransform, ECS::FEntity DropTarget, bool bAttachToTarget)>;

    class EDITOR_API FEditorAssetDropRegistry
    {
    public:

        static FEditorAssetDropRegistry& Get();

        // bPlacesEntity marks a handler that spawns a new entity with no target, which a viewport drag previews live.
        void Register(FName AssetClass, FEditorAssetDropHandler Handler, bool bPlacesEntity = false);

        const FEditorAssetDropHandler* FindHandler(FName AssetClass) const;

        NODISCARD bool PlacesEntity(FName AssetClass) const;

        // Runs the asset's handler, or with none sets the one object field on DropTarget's components that takes it.
        ECS::FEntity Drop(CWorld* World, CObject* Asset, const FTransform& SpawnTransform, ECS::FEntity DropTarget, bool bAttachToTarget) const;

        // Fills the editable object field on DropTarget that takes Asset, preferring an empty one when several do.
        static ECS::FEntity AssignToComponentField(CWorld* World, CObject* Asset, ECS::FEntity DropTarget);

    private:

        struct FEntry
        {
            FEditorAssetDropHandler Handler;
            bool                    bPlacesEntity = false;
        };

        // The class's own entry, else the nearest super class's, so one registration covers its subclasses.
        const FEntry* FindEntry(FName AssetClass) const;

        THashMap<FName, FEntry> Handlers;
    };
}
