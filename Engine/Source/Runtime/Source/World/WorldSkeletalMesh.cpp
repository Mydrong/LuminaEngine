#include "RuntimePCH.h"

#include "Animation/Pose.h"
#include "Animation/SkeletalMeshUtils.h"
#include "World/ECS/Registry.h"
#include "World/Entity/Components/FollowerPoseComponent.h"
#include "World/Entity/Components/SkeletalMeshComponent.h"
#include "World/Entity/Components/SocketAttachmentComponent.h"
#include "World/Entity/Components/TransformComponent.h"
#include "World/Entity/EntityUtils.h"
#include "World/World.h"

namespace Lumina
{
    void CWorld::AttachEntityToSocket(ECS::FEntity Child, ECS::FEntity Parent,
        const FName& SocketOrBone)
    {
        if (!EntityRegistry.IsValid(Child) || !EntityRegistry.IsValid(Parent) || Child == Parent)
        {
            return;
        }

        // The socket system overwrites the local transform anyway, and the snap avoids a stale frame.
        ECS::Utils::ReparentEntity(EntityRegistry, Child, Parent, /*bPreserveWorld*/ false);

        SSocketAttachmentComponent& Attachment = EntityRegistry.EmplaceOrReplace<SSocketAttachmentComponent>(Child);
        Attachment.SocketName = SocketOrBone;

        FMatrix4 SocketTransform;
        STransformComponent* Transform = EntityRegistry.TryGet<STransformComponent>(Child);
        if (Transform && SkeletalUtils::GetEntitySocketTransform(EntityRegistry, Parent, SocketOrBone, SocketTransform))
        {
            Transform->SetLocalTransform(FTransform(SocketTransform * Attachment.RelativeTransform.GetMatrix()));
        }
    }

    void CWorld::DetachEntityFromSocket(ECS::FEntity Entity)
    {
        if (!EntityRegistry.IsValid(Entity))
        {
            return;
        }

        EntityRegistry.Remove<SSocketAttachmentComponent>(Entity);
        ECS::Utils::ReparentEntity(EntityRegistry, Entity, ECS::NullEntity, /*bPreserveWorld*/ true);
    }

    bool CWorld::HasSocket(ECS::FEntity Entity, const FName& SocketOrBone)
    {
        return SkeletalUtils::EntityHasSocket(EntityRegistry, Entity, SocketOrBone);
    }

    FVector3 CWorld::GetSocketLocation(ECS::FEntity Entity, const FName& SocketOrBone)
    {
        FMatrix4 SocketTransform;
        if (!SkeletalUtils::GetSocketWorldTransform(EntityRegistry, Entity, SocketOrBone, SocketTransform))
        {
            return FVector3(0.0f);
        }
        return FVector3(SocketTransform[3]);
    }

    FQuat CWorld::GetSocketRotation(ECS::FEntity Entity, const FName& SocketOrBone)
    {
        FMatrix4 SocketTransform;
        if (!SkeletalUtils::GetSocketWorldTransform(EntityRegistry, Entity, SocketOrBone, SocketTransform))
        {
            return FQuat::Identity();
        }

        FVector3 Translation; FQuat Rotation; FVector3 Scale;
        AnimPose::DecomposeTRS(SocketTransform, Translation, Rotation, Scale);
        return Rotation;
    }

    FName CWorld::GetBoneName(ECS::FEntity Entity, int32 BoneIndex)
    {
        if (!EntityRegistry.IsValid(Entity))
        {
            return FName();
        }

        const SSkeletalMeshComponent* Mesh = EntityRegistry.TryGet<SSkeletalMeshComponent>(Entity);
        if (Mesh == nullptr)
        {
            return FName();
        }

        const FSkeletonResource* Skeleton = SkeletalUtils::GetSkeleton(*Mesh);
        if (Skeleton == nullptr || !Skeleton->IsBoneIndexValid(BoneIndex))
        {
            return FName();
        }
        return Skeleton->GetBone(BoneIndex).Name;
    }

    int32 CWorld::GetBoneIndex(ECS::FEntity Entity, const FName& BoneName)
    {
        if (!EntityRegistry.IsValid(Entity))
        {
            return Constants::kIndexNone;
        }

        const SSkeletalMeshComponent* Mesh = EntityRegistry.TryGet<SSkeletalMeshComponent>(Entity);
        if (Mesh == nullptr)
        {
            return Constants::kIndexNone;
        }

        const FSkeletonResource* Skeleton = SkeletalUtils::GetSkeleton(*Mesh);
        return Skeleton ? Skeleton->FindBoneIndex(BoneName) : Constants::kIndexNone;
    }

    FName CWorld::FindClosestBone(ECS::FEntity Entity, FVector3 WorldLocation)
    {

        const int32 BoneIndex = SkeletalUtils::FindClosestBone(EntityRegistry, Entity, WorldLocation);
        return GetBoneName(Entity, BoneIndex);
    }

    void CWorld::SetLeaderPose(ECS::FEntity Follower, ECS::FEntity Leader)
    {
        if (!EntityRegistry.IsValid(Follower) || Follower == Leader)
        {
            return;
        }

        if (!EntityRegistry.HasAll<SSkeletalMeshComponent>(Follower))
        {
            LOG_WARN("SetLeaderPose: entity {} has no skeletal mesh to drive.", Follower.Value);
            return;
        }

        SFollowerPoseComponent& Link = EntityRegistry.EmplaceOrReplace<SFollowerPoseComponent>(Follower);
        Link.Leader = Leader;
    }

    void CWorld::ClearLeaderPose(ECS::FEntity Follower)
    {
        if (EntityRegistry.IsValid(Follower))
        {
            EntityRegistry.Remove<SFollowerPoseComponent>(Follower);
        }
    }

    ECS::FEntity CWorld::GetLeaderPose(ECS::FEntity Follower)
    {
        if (!EntityRegistry.IsValid(Follower))
        {
            return ECS::NullEntity;
        }
        return SkeletalUtils::ResolvePoseLeader(EntityRegistry, Follower);
    }
}
