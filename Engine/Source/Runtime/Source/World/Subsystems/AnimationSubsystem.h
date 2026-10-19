#pragma once

#include "Containers/Name.h"
#include "Core/Object/ObjectMacros.h"
#include "World/ECS/Entity.h"
#include "Core/Math/Quat/Quat.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "World/Subsystems/WorldSubsystem.h"
#include "AnimationSubsystem.generated.h"

namespace Lumina
{
    class CAnimation;
    class CAnimationMontage;

    // Animation on an entity's graph and montages, and its skeleton's bones and sockets.
    REFLECT()
    class RUNTIME_API CAnimationSubsystem : public CWorldSubsystem
    {
        GENERATED_BODY()

    public:

        //~ The simple single clip backend, SSimpleAnimationComponent.

        /** Adds the component if the entity has none, which is why this is the one call that cannot no-op. */
        FUNCTION()
        void Play(ECS::FEntity Entity, CAnimation* Clip, bool bLoop = false,
            float Speed = 1.0f);

        FUNCTION()
        void Stop(ECS::FEntity Entity);

        FUNCTION()
        void Pause(ECS::FEntity Entity);

        FUNCTION()
        void Resume(ECS::FEntity Entity);

        FUNCTION()
        bool IsPlaying(ECS::FEntity Entity);

        FUNCTION()
        bool IsFinished(ECS::FEntity Entity);

        FUNCTION()
        void SetSpeed(ECS::FEntity Entity, float Speed);

        FUNCTION()
        void SetTime(ECS::FEntity Entity, float Time);

        FUNCTION()
        float GetTime(ECS::FEntity Entity);

        //~ Graph parameters, which live as fields on the graph's own parameter struct.

        FUNCTION()
        void SetFloat(ECS::FEntity Entity, const FName& Name, float Value);

        FUNCTION()
        float GetFloat(ECS::FEntity Entity, const FName& Name, float Default = 0.0f);

        FUNCTION()
        void SetBool(ECS::FEntity Entity, const FName& Name, bool bValue);

        FUNCTION()
        bool GetBool(ECS::FEntity Entity, const FName& Name, bool bDefault = false);

        FUNCTION()
        bool HasParameter(ECS::FEntity Entity, const FName& Name);

        //~ Montages. The graph must contain a matching slot node for anything to show.

        /** Zero when it did not start. An empty section starts at the montage's first one. */
        FUNCTION()
        uint32 PlayMontage(ECS::FEntity Entity, CAnimationMontage* Montage,
            float PlayRate = 1.0f, const FName& Section = FName());

        /** A null montage stops every one playing on the entity. */
        FUNCTION()
        void StopMontage(ECS::FEntity Entity, CAnimationMontage* Montage,
            float BlendOutTime = -1.0f);

        FUNCTION()
        bool JumpToMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage,
            const FName& Section);

        FUNCTION()
        bool SetNextMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage,
            const FName& Section);

        /** A null montage asks whether any montage is playing at all. */
        FUNCTION()
        bool IsMontagePlaying(ECS::FEntity Entity, CAnimationMontage* Montage);

        FUNCTION()
        float GetMontagePosition(ECS::FEntity Entity, CAnimationMontage* Montage);

        FUNCTION()
        float GetMontageWeight(ECS::FEntity Entity, CAnimationMontage* Montage);

        FUNCTION()
        void SetMontagePlayRate(ECS::FEntity Entity, CAnimationMontage* Montage,
            float PlayRate);

        FUNCTION()
        FName GetMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage);

        /** Parents Child under Parent and keeps it glued to the named socket each frame. */
        FUNCTION()
        void AttachEntityToSocket(ECS::FEntity Child, ECS::FEntity Parent,
            const FName& SocketOrBone);

        /** Stops following the socket and detaches to the world root, preserving the world transform. */
        FUNCTION()
        void DetachEntityFromSocket(ECS::FEntity Entity);

        FUNCTION()
        bool HasSocket(ECS::FEntity Entity, const FName& SocketOrBone);

        /** World-space socket location, zero when it does not resolve. */
        FUNCTION()
        FVector3 GetSocketLocation(ECS::FEntity Entity, const FName& SocketOrBone);

        /** World-space socket rotation, identity when it does not resolve. */
        FUNCTION()
        FQuat GetSocketRotation(ECS::FEntity Entity, const FName& SocketOrBone);

        /** Bone name for a skeleton bone index such as a hit result's, NAME_None when out of range. */
        FUNCTION()
        FName GetBoneName(ECS::FEntity Entity, int32 BoneIndex);

        FUNCTION()
        int32 GetBoneIndex(ECS::FEntity Entity, const FName& BoneName);

        /** Bone origin nearest WorldLocation, which approximates the hit bone on a single-body mesh. */
        FUNCTION()
        FName FindClosestBone(ECS::FEntity Entity, FVector3 WorldLocation);

        /** Drives Follower's skeletal mesh from Leader's pose instead of its own animation. A null Leader follows the nearest parent with a skeletal mesh. */
        FUNCTION()
        void SetLeaderPose(ECS::FEntity Follower, ECS::FEntity Leader);

        /** Stops Follower copying a leader, so its own animation drives it again. */
        FUNCTION()
        void ClearLeaderPose(ECS::FEntity Follower);

        /** The entity whose pose Follower ends up copying, null when it follows nothing. */
        FUNCTION()
        ECS::FEntity GetLeaderPose(ECS::FEntity Follower);
    };
}
