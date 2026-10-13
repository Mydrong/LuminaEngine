#pragma once

#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Quat/Quat.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "World/ECS/Entity.h"
#include "SkeletalMeshLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    /** Socket and bone lookups, and leader poses. A SocketOrBone is a socket authored on the asset, or a raw bone name. */
    REFLECT()
    class RUNTIME_API CSkeletalMeshLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** Parents Child under Parent and keeps it glued to the named socket each frame. */
        FUNCTION()
        static void AttachEntityToSocket(CWorld* World, ECS::FEntity Child, ECS::FEntity Parent,
            const FName& SocketOrBone);

        /** Stops following the socket and detaches to the world root, preserving the world transform. */
        FUNCTION()
        static void DetachEntityFromSocket(CWorld* World, ECS::FEntity Entity);

        FUNCTION()
        static bool HasSocket(CWorld* World, ECS::FEntity Entity, const FName& SocketOrBone);

        /** World-space socket location, zero when it does not resolve. */
        FUNCTION()
        static FVector3 GetSocketLocation(CWorld* World, ECS::FEntity Entity, const FName& SocketOrBone);

        /** World-space socket rotation, identity when it does not resolve. */
        FUNCTION()
        static FQuat GetSocketRotation(CWorld* World, ECS::FEntity Entity, const FName& SocketOrBone);

        /** Bone name for a skeleton bone index such as a hit result's, NAME_None when out of range. */
        FUNCTION()
        static FName GetBoneName(CWorld* World, ECS::FEntity Entity, int32 BoneIndex);

        FUNCTION()
        static int32 GetBoneIndex(CWorld* World, ECS::FEntity Entity, const FName& BoneName);

        /** Bone origin nearest WorldLocation, which approximates the hit bone on a single-body mesh. */
        FUNCTION()
        static FName FindClosestBone(CWorld* World, ECS::FEntity Entity, FVector3 WorldLocation);

        /** Drives Follower's skeletal mesh from Leader's pose instead of its own animation. A null Leader follows the nearest parent with a skeletal mesh. */
        FUNCTION()
        static void SetLeaderPose(CWorld* World, ECS::FEntity Follower, ECS::FEntity Leader);

        /** Stops Follower copying a leader, so its own animation drives it again. */
        FUNCTION()
        static void ClearLeaderPose(CWorld* World, ECS::FEntity Follower);

        /** The entity whose pose Follower ends up copying, null when it follows nothing. */
        FUNCTION()
        static ECS::FEntity GetLeaderPose(CWorld* World, ECS::FEntity Follower);
    };
}
