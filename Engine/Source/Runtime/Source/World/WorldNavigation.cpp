#include "RuntimePCH.h"

#include "World/World.h"

#include "World/Entity/Systems/NavMeshSystem.h"

namespace Lumina
{
    bool CWorld::IsNavigationReady(FName Agent)
    {
        return Nav::IsReady(this, Agent);
    }

    int32 CWorld::FindPath(FVector3 Start, FVector3 End, FVector3* Corners,
        int32 MaxCorners, FName Agent)
    {
        FNavPath Path;
        if (Corners == nullptr || MaxCorners <= 0 || !Nav::FindPath(this, Start, End, MaxCorners, Path, Agent)
            || !Path.bValid)
        {
            return 0;
        }

        int32 Count = (int32)Path.Corners.size();
        if (Count > MaxCorners)
        {
            Count = MaxCorners;
        }
        for (int32 i = 0; i < Count; ++i)
        {
            Corners[i] = Path.Corners[i];
        }
        return Count;
    }

    bool CWorld::IsPathPartial(FVector3 Start, FVector3 End, int32 MaxCorners, FName Agent)
    {
        FNavPath Path;
        if (!Nav::FindPath(this, Start, End, MaxCorners, Path, Agent) || !Path.bValid)
        {
            return true;
        }
        return Path.bPartial || Path.bTruncated || (int32)Path.Corners.size() > MaxCorners;
    }

    ENavPathResult CWorld::GetPathResult(FVector3 Start, FVector3 End, int32 MaxCorners, FName Agent)
    {
        FNavPath Path;
        Nav::FindPath(this, Start, End, MaxCorners, Path, Agent);

        // A route longer than the caller's buffer arrives valid but short, which is a truncation either way.
        if (Path.bValid && Path.Result == ENavPathResult::Success && MaxCorners > 0
            && (int32)Path.Corners.size() > MaxCorners)
        {
            return ENavPathResult::Truncated;
        }
        return Path.Result;
    }

    SNavPoint CWorld::ProjectPointToNavigation(FVector3 Point, FVector3 Extents, FName Agent)
    {
        SNavPoint Result;
        FVector3 Projected;
        if (Nav::ProjectPoint(this, Point, Extents, Projected, Agent))
        {
            Result.bFound = true;
            Result.Point = Projected;
        }
        return Result;
    }

    SNavRaycastResult CWorld::NavRaycast(FVector3 Start, FVector3 End, FName Agent)
    {
        SNavRaycastResult Result;
        Nav::Raycast(this, Start, End, Result, Agent);
        return Result;
    }

    bool CWorld::IsWalkableLine(FVector3 From, FVector3 To, FName Agent)
    {
        return Nav::IsWalkableLine(this, From, To, Agent);
    }

    SNavPoint CWorld::FindRandomReachablePoint(FVector3 Origin, float Radius, FName Agent)
    {
        SNavPoint Result;
        FVector3 Found;
        if (Nav::FindRandomReachablePoint(this, Origin, Radius, Found, Agent))
        {
            Result.bFound = true;
            Result.Point = Found;
        }
        return Result;
    }

    bool CWorld::IsReachable(FVector3 From, FVector3 To, FName Agent)
    {
        return Nav::IsReachable(this, From, To, Agent);
    }

    float CWorld::GetPathLength(FVector3 From, FVector3 To, FName Agent)
    {
        return Nav::PathLength(this, From, To, Agent);
    }

    int32 CWorld::RequestNavigationRebuild()
    {
        return Nav::RequestRebuild(this);
    }

    void CWorld::DrawNavPath(FVector3 From, FVector3 To, FVector4 Color, float Duration, FName Agent)
    {
        Nav::DrawDebugPath(this, From, To, Color, Duration, Agent);
    }
}
