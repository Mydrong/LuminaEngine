#include "RuntimePCH.h"

#include "World/ECS/Registry.h"
#include "World/Entity/Systems/SystemSingletons.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        const FResolvedSceneView* ViewOf(CWorld& World)
        {
            return World.GetResolvedView();
        }
    }

    namespace
    {
        // A hit this close short of the point is the point's own surface, not something in front of it.
        constexpr float kOcclusionSlack = 0.35f;

        bool IsUnoccluded(CWorld& World, const FVector3& Eye, const FVector3& Point, float Distance, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB)
        {
            const SRayResult Hit = World.Raycast(Eye, Point, IgnoreA);
            return !Hit.bHit || ECS::FEntity(Hit.Entity) == IgnoreB || Hit.Distance >= Distance - kOcclusionSlack;
        }
    }

    FVector3 CWorld::GetViewPosition()
    {
        const FResolvedSceneView* View = ViewOf(*this);
        return View != nullptr ? View->ViewVolume.GetViewPosition() : FVector3(0.0f);
    }

    FVector3 CWorld::GetViewForward()
    {
        const FResolvedSceneView* View = ViewOf(*this);
        return View != nullptr ? View->ViewVolume.GetForwardVector() : FVector3(0.0f, 0.0f, 1.0f);
    }

    FVector3 CWorld::GetViewRight()
    {
        const FResolvedSceneView* View = ViewOf(*this);
        return View != nullptr ? View->ViewVolume.GetRightVector() : FVector3(1.0f, 0.0f, 0.0f);
    }

    FVector3 CWorld::GetViewUp()
    {
        const FResolvedSceneView* View = ViewOf(*this);
        return View != nullptr ? View->ViewVolume.GetUpVector() : FVector3(0.0f, 1.0f, 0.0f);
    }

    float CWorld::GetFieldOfView()
    {
        const FResolvedSceneView* View = ViewOf(*this);
        return View != nullptr ? View->ViewVolume.GetFOV() : 0.0f;
    }

    uint32 CWorld::PlayCameraShake(SCameraShakeParams Params)
    {
        return SCameraSystem::PlayCameraShake(EntityRegistry, Params);
    }

    uint32 CWorld::PlayImpactCameraShake(float Intensity, float Duration)
    {
        SCameraShakeParams Params;
        Params.LocationAmplitude = FVector3(Intensity * 0.04f, Intensity * 0.04f, Intensity * 0.02f);
        Params.RotationAmplitude = FVector3(Intensity * 1.5f, Intensity * 1.5f, Intensity * 0.6f);
        Params.Frequency = 18.0f;
        Params.Duration = Duration;
        Params.BlendInTime = 0.03f;
        Params.BlendOutTime = Math::Min(Duration, 0.25f);
        return PlayCameraShake(Params);
    }

    void CWorld::StopCameraShake(uint32 ShakeHandle)
    {
        {
            SCameraSystem::StopCameraShake(EntityRegistry, ShakeHandle);
        }
    }

    void CWorld::StopAllCameraShakes()
    {
        {
            SCameraSystem::StopAllCameraShakes(EntityRegistry);
        }
    }

    FVector2 CWorld::GetViewportSize()
    {
        IRenderScene* Scene = GetRenderer();
        if (Scene == nullptr)
        {
            return FVector2(0.0f);
        }
        const FUIntVector2 Extent = Scene->GetRenderExtent();
        return FVector2((float)Extent.x, (float)Extent.y);
    }

    SScreenProjection CWorld::WorldToScreen(FVector3 WorldLocation)
    {
        SScreenProjection Result;
        const FResolvedSceneView* View = ViewOf(*this);
        const FVector2 Viewport = GetViewportSize();
        if (View == nullptr || Viewport.x <= 0.0f || Viewport.y <= 0.0f)
        {
            return Result;
        }

        Result.bOnScreen = View->ViewVolume.WorldToScreen(WorldLocation, Viewport, Result.Position, Result.Depth);
        return Result;
    }

    SWorldRay CWorld::ScreenToWorldRay(FVector2 ScreenPosition)
    {
        SWorldRay Result;
        const FResolvedSceneView* View = ViewOf(*this);
        const FVector2 Viewport = GetViewportSize();
        if (View == nullptr || Viewport.x <= 0.0f || Viewport.y <= 0.0f)
        {
            return Result;
        }

        View->ViewVolume.ScreenToWorldRay(ScreenPosition, Viewport, Result.Origin, Result.Direction);
        Result.bValid = true;
        return Result;
    }

    SWorldRay CWorld::ViewportCenterRay()
    {
        const FVector2 Viewport = GetViewportSize();
        return ScreenToWorldRay(FVector2(Viewport.x * 0.5f, Viewport.y * 0.5f));
    }

    FVector3 CWorld::DeprojectScreenToWorld(FVector2 ScreenPosition, float WorldDistance)
    {
        const SWorldRay Ray = ScreenToWorldRay(ScreenPosition);
        return Ray.bValid ? (Ray.Origin + Ray.Direction * WorldDistance) : FVector3(0.0f);
    }

    bool CWorld::IsPointVisible(FVector3 Point, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB, float MaxDistance)
    {
        const FVector3 Eye = GetViewPosition();
        const float Distance = Math::Length(Point - Eye);
        if (Distance > MaxDistance)
        {
            return false;
        }

        const SScreenProjection Projected = WorldToScreen(Point);
        const FVector2 Viewport = GetViewportSize();
        if (!Projected.bOnScreen || Projected.Depth <= 0.0f)
        {
            return false;
        }
        if (Projected.Position.x < 0.0f || Projected.Position.y < 0.0f || Projected.Position.x > Viewport.x || Projected.Position.y > Viewport.y)
        {
            return false;
        }

        return IsUnoccluded(*this, Eye, Point, Distance, IgnoreA, IgnoreB);
    }

    bool CWorld::IsPointVisibleFrom(FVector3 Eye, FVector3 Forward, float FovDegrees, float Aspect,
        FVector3 Point, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB, float MaxDistance)
    {
        const FVector3 ToPoint = Point - Eye;
        const float Distance = Math::Length(ToPoint);
        if (Distance > MaxDistance || Distance < 1e-3f || Math::Length(Forward) < 1e-3f)
        {
            return false;
        }

        // Projected onto the view's own axes, which is the frustum test without a viewport.
        const FVector3 ViewForward = Math::Normalize(Forward);
        FVector3 Right = Math::Cross(FVector3(0.0f, 1.0f, 0.0f), ViewForward);
        Right = Math::Length(Right) > 1e-3f ? Math::Normalize(Right) : FVector3(1.0f, 0.0f, 0.0f);
        const FVector3 Up = Math::Cross(ViewForward, Right);
        const float Depth = Math::Dot(ToPoint, ViewForward);
        if (Depth <= 0.0f)
        {
            return false;
        }
        const float TanHalfV = Math::Tan(Math::Radians(FovDegrees * 0.5f));
        const float TanHalfH = TanHalfV * Aspect;
        if (Math::Abs(Math::Dot(ToPoint, Right)) > Depth * TanHalfH || Math::Abs(Math::Dot(ToPoint, Up)) > Depth * TanHalfV)
        {
            return false;
        }

        return IsUnoccluded(*this, Eye, Point, Distance, IgnoreA, IgnoreB);
    }
}
