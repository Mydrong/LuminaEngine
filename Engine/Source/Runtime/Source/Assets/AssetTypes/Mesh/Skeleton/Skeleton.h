#pragma once

#include "Core/Object/Object.h"
#include "Assets/AssetTypes/Mesh/MeshSocket.h"
#include "Renderer/MeshData.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Memory/SmartPtr.h"
#include "Renderer/SkeletonResource.h"
#include <atomic>
#include "Skeleton.generated.h"

namespace Lumina
{
    class CSkeletalMesh;

    REFLECT()
    class RUNTIME_API CSkeleton : public CObject
    {
        friend class CMeshImporter;

        GENERATED_BODY()

    public:
        void Serialize(FArchive& Ar) override;
        void PostPropertyChange(FProperty* ChangedProperty) override;

        bool IsAsset() const override { return true; }

        FSkeletonResource* GetSkeletonResource() const { return SkeletonResource.get(); }

        void SetSkeletonResource(TUniquePtr<FSkeletonResource>&& NewResource);

        void ComputeBindPoseSkinningMatrices(TVector<FMatrix4>& OutMatrices) const;

        const FMeshSocket* FindSocket(const FName& SocketName) const { return FindSocketByName(Sockets, SocketName); }

        /** Bone index the named socket rides on; Constants::kIndexNone when the socket or its bone doesn't exist. */
        int32 FindSocketBoneIndex(const FName& SocketName) const;

        // Bumped whenever the sockets or the bones change, so anything built around the socket bones knows to rebuild.
        uint32 GetSocketGeneration() const { return SocketGeneration.load(std::memory_order_relaxed); }

        void CollectSocketBones(TVector<int32>& OutBones) const;

        PROPERTY(Editable, Category = "Preview")
        TStrongObjectPtr<CSkeletalMesh> PreviewMesh;

        /** Named attach points riding skeleton bones. */
        PROPERTY(Editable, Category = "Sockets")
        TVector<FMeshSocket> Sockets;

    private:

        void BumpSocketGeneration() { SocketGeneration.fetch_add(1, std::memory_order_relaxed); }

        TUniquePtr<FSkeletonResource> SkeletonResource;

        std::atomic<uint32> SocketGeneration{ 1 };
    };
}
