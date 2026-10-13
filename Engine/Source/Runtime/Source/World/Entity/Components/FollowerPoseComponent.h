#pragma once

#include "Containers/Vector.h"
#include "Core/Math/Math.h"
#include "Core/Object/ObjectMacros.h"
#include "Platform/GenericPlatform.h"
#include "World/ECS/Entity.h"
#include "FollowerPoseComponent.generated.h"

namespace Lumina
{
    // Drives this mesh from another entity's pose, so a modular character costs one evaluation.
    REFLECT(Component, Category = "Animation")
    struct RUNTIME_API SFollowerPoseComponent
    {
        GENERATED_BODY()

        /** Entity whose pose this mesh copies. Left empty, it is the nearest parent with a skeletal mesh. */
        PROPERTY(Editable, Category = "Follower Pose")
        ECS::FEntity Leader = ECS::NullEntity;

        /** Bones the leader has no match for keep their bind pose rather than following anything. */
        PROPERTY(Editable, Category = "Follower Pose")
        bool bWarnOnMissingBones = true;

        // Follower bone to leader bone, by name. Constants::kIndexNone where the leader has no such bone. Transient.
        TVector<int32> BoneMap;

        // Leader bind times follower inverse bind, which is what lands a copy between two skeletons.
        TVector<FMatrix4> BindFixups;

        // Skeletons the map was built against; a change on either side rebuilds it. Transient.
        const void* MappedLeaderSkeleton = nullptr;
        const void* MappedFollowerSkeleton = nullptr;

        // Skips the copy while the same leader's pose has not moved. Transient.
        ECS::FEntity CopiedLeader = ECS::NullEntity;
        uint32 LastLeaderPoseSerial = 0;

        // Set while this mesh has the leader's skeleton, so the renderer skins it from the leader's bones. Transient.
        bool bSharesLeaderPose = false;

        // Set once the map came back empty, so the warning fires once rather than every frame. Transient.
        bool bWarnedNoMatch = false;
    };
}
