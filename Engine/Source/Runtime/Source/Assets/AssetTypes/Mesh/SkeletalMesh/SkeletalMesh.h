#pragma once

#include "Animation/BoneLOD.h"
#include "Assets/AssetTypes/Mesh/Mesh.h"
#include "Core/Threading/Thread.h"
#include <atomic>
#include "SkeletalMesh.generated.h"

namespace Lumina
{
    class CSkeleton;
    struct FSkeletonResource;

    REFLECT()
    class RUNTIME_API CSkeletalMesh : public CMesh
    {
        GENERATED_BODY()

        friend class CMeshImporter;

    public:

        void Serialize(FArchive& Ar) override;
        void PostLoad() override;
        void PostPropertyChange(FProperty* ChangedProperty) override;
        bool IsAsset() const override { return true; }
        bool IsSkinned() const override { return true; }

        // Rewrites the joint indices into Skeleton's bone order, matching by name, after Skeleton is reassigned.
        void RemapJointIndicesToSkeleton();

        // The order this mesh's poses evaluate in against Skeleton, which bone LOD cuts; null without skin data. Safe from any thread.
        const BoneLOD::FBoneLODLayout* GetBoneLODLayout(const FSkeletonResource* Skeleton) const;

        PROPERTY(Editable, Category = "Skeleton")
        TStrongObjectPtr<CSkeleton> Skeleton;

        // What a runtime merge built this from, held so the parts stay loaded while anything wears it.
        TVector<TStrongObjectPtr<CSkeletalMesh>> MergeSources;

    private:

        struct FBoneLODLayoutEntry
        {
            uint64             LayoutKey        = 0;
            const void*        Resource         = nullptr;
            size_t             PaletteEntries   = 0;
            uint32             SocketGeneration = 0;
            TUniquePtr<BoneLOD::FBoneLODLayout> Layout;

            bool Matches(uint64 InLayoutKey, const void* InResource, size_t InPaletteEntries, uint32 InSocketGeneration) const
            {
                return LayoutKey == InLayoutKey && Resource == InResource && PaletteEntries == InPaletteEntries
                    && SocketGeneration == InSocketGeneration;
            }
        };

        TStrongObjectPtr<CSkeleton> SkeletonJointIndicesAddress;

        // Never freed before the mesh, because animation threads hold pointers into them without a lock.
        mutable TVector<TUniquePtr<FBoneLODLayoutEntry>> BoneLODLayouts;
        mutable std::atomic<const FBoneLODLayoutEntry*>  ActiveBoneLODLayout{ nullptr };
        mutable FMutex                                   BoneLODMutex;
    };
}
