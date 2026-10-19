#pragma once

#include "AI/Navigation/NavTypes.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "World/Subsystems/WorldSubsystem.h"
#include "NavigationSubsystem.generated.h"

namespace Lumina
{
    // Navigation queries. With no baked navmesh every query reports not found, and an empty Agent means the infantry default.
    REFLECT()
    class RUNTIME_API CNavigationSubsystem : public CWorldSubsystem
    {
        GENERATED_BODY()

    public:

        /** True once a navmesh has been baked and finished hydrating, so queries can succeed. */
        FUNCTION()
        bool IsReady(FName Agent = FName());

        /** Writes the route's corners into the caller's buffer and returns how many it wrote. */
        FUNCTION()
        int32 FindPath(FVector3 Start, FVector3 End, FVector3* Corners, int32 MaxCorners, FName Agent = FName());

        /** Whether the last route between these points stops short of the goal, for any reason. */
        FUNCTION()
        bool IsPathPartial(FVector3 Start, FVector3 End, int32 MaxCorners, FName Agent = FName());

        // Why a route between these points ended as it did, so a failure can say which part failed.
        FUNCTION()
        ENavPathResult GetPathResult(FVector3 Start, FVector3 End, int32 MaxCorners, FName Agent = FName());

        /** Snaps onto the nearest walkable surface within the half-extents search box. */
        FUNCTION()
        SNavPoint ProjectPoint(FVector3 Point, FVector3 Extents, FName Agent = FName());

        /** Walks the surface toward End and reports where a wall stopped it. */
        FUNCTION()
        SNavRaycastResult Raycast(FVector3 Start, FVector3 End, FName Agent = FName());

        /** Whether the straight line stays on walkable surface the whole way, far cheaper than a path. */
        FUNCTION()
        bool IsWalkableLine(FVector3 From, FVector3 To, FName Agent = FName());

        FUNCTION()
        SNavPoint FindRandomReachablePoint(FVector3 Origin, float Radius, FName Agent = FName());

        /** Whether a complete route exists, so a partial one reports false. */
        FUNCTION()
        bool IsReachable(FVector3 From, FVector3 To, FName Agent = FName());

        /** Total route length in world units, or a negative value when unreachable. */
        FUNCTION()
        float PathLength(FVector3 From, FVector3 To, FName Agent = FName());

        /** Flags every navmesh volume for an async rebuild and returns how many it flagged. */
        FUNCTION()
        int32 RequestRebuild();

        FUNCTION()
        void DrawPath(FVector3 From, FVector3 To, FVector4 Color, float Duration = 0.0f, FName Agent = FName());
    };
}
