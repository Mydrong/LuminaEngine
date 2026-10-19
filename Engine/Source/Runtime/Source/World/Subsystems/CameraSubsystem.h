#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "World/ECS/Entity.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Systems/CameraSystem.h"
#include "World/Subsystems/WorldSubsystem.h"
#include "CameraSubsystem.generated.h"

namespace Lumina
{
    // The resolved view, projections, the active camera and shakes.
    REFLECT()
    class RUNTIME_API CCameraSubsystem : public CWorldSubsystem
    {
        GENERATED_BODY()

    public:

        // Eases from the current view over BlendTime seconds, where 0 snaps. An entity without a camera is ignored.
        FUNCTION()
        void SetActiveCamera(ECS::FEntity Entity, float BlendTime = 0.0f, ECameraBlendFunction Function = ECameraBlendFunction::EaseInOut);

        FUNCTION()
        SCameraComponent* GetActiveCamera() const;

        FUNCTION()
        ECS::FEntity GetActiveCameraEntity() const;

        // Taken from the resolved view, so an active blend and any running shake are already folded in.
        FUNCTION()
        FVector3 GetViewPosition();

        FUNCTION()
        FVector3 GetViewForward();

        FUNCTION()
        FVector3 GetViewRight();

        FUNCTION()
        FVector3 GetViewUp();

        /** Vertical field of view in degrees, zero before anything has been rendered. */
        FUNCTION()
        float GetFieldOfView();

        /** Returns a handle to stop the shake with. Several shakes sum on the rendered view. */
        FUNCTION()
        uint32 PlayShake(SCameraShakeParams Params);

        /** An impact or explosion kick, where Intensity near one is a light bump and five a heavy blast. */
        FUNCTION()
        uint32 PlayImpactShake(float Intensity, float Duration = 0.4f);

        FUNCTION()
        void StopShake(uint32 ShakeHandle);

        FUNCTION()
        void StopAllShakes();

        /** Viewport size in pixels, or zero when the world has no renderer such as a dedicated server. */
        FUNCTION()
        FVector2 GetViewportSize();

        /** Projects a world point to pixel coordinates, origin top-left. Check bOnScreen before using it. */
        FUNCTION()
        SScreenProjection WorldToScreen(FVector3 WorldLocation);

        /** Pixel coordinates to a world ray, the aiming and picking primitive. */
        FUNCTION()
        SWorldRay ScreenToWorldRay(FVector2 ScreenPosition);

        /** ScreenToWorldRay through the center of the viewport, which is where a crosshair sits. */
        FUNCTION()
        SWorldRay ViewportCenterRay();

        /** A point WorldDistance along the ray through ScreenPosition, measured from the near plane. */
        FUNCTION()
        FVector3 DeprojectScreenToWorld(FVector2 ScreenPosition, float WorldDistance);

        // On screen in the active view, within MaxDistance, and with nothing solid in between. IgnoreA is the viewer's body, IgnoreB the target's.
        FUNCTION()
        bool IsPointVisible(FVector3 Point, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB, float MaxDistance);

        // The same test from any eye, which is how a server judges what each player's camera sees.
        FUNCTION()
        bool IsPointVisibleFrom(FVector3 Eye, FVector3 Forward, float FovDegrees, float Aspect,
            FVector3 Point, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB, float MaxDistance);
    };
}
