#pragma once

#include "Containers/Vector.h"
#include "Memory/SmartPtr.h"
#include "Platform/GenericPlatform.h"
#include "Renderer/SkeletonResource.h"

namespace Lumina
{
    struct FMeshResource;
}

namespace Lumina::BoneLOD
{
    // The order a mesh's poses are evaluated in, bones that still show at a distance first, so any prefix is a valid partial skeleton.
    struct FBoneLODLayout
    {
        // Per pose bone, how far the mesh strays once that bone and everything below it hold their bind pose.
        TVector<float> Errors;

        // The skeleton in pose order, or null when the asset order already is that order.
        TUniquePtr<FSkeletonResource> PoseSkeleton;

        TVector<int32> PoseToAsset;
        TVector<int32> AssetToPose;
    };

    // AlwaysAnimated bones and their ancestors sort first and are never cut, which is what keeps sockets moving.
    RUNTIME_API TUniquePtr<FBoneLODLayout> BuildLayout(const FMeshResource& Mesh, const FSkeletonResource& Skeleton, const TVector<int32>& AlwaysAnimated);

    // Per skeleton bone, how far in mesh units this mesh strays once that bone and everything below it hold their bind pose.
    RUNTIME_API TVector<float> ComputeBoneErrors(const FMeshResource& Mesh, const FSkeletonResource& Skeleton);

    // A parents-first order with the bones that still matter at a distance first, so every prefix is a valid partial skeleton.
    RUNTIME_API TVector<int32> SortBonesByError(const FSkeletonResource& Skeleton, const TVector<float>& BoneErrors);

    // Rewrites the skeleton into NewToOld order and returns the inverse mapping.
    RUNTIME_API TVector<int32> ReorderSkeleton(FSkeletonResource& Skeleton, const TVector<int32>& NewToOld);

    // Carries per-bone errors through a reorder, so OutErrors[OldToNew[i]] is InErrors[i].
    RUNTIME_API TVector<float> RemapBoneErrors(const TVector<float>& BoneErrors, const TVector<int32>& OldToNew);

    // How many leading bones to animate when Tolerance mesh units of error go unnoticed, never cutting below MinBones.
    RUNTIME_API int32 CountActiveBones(const TVector<float>& BoneErrors, int32 NumBones, float Tolerance, int32 MinBones);
}
