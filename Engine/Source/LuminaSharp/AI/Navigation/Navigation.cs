using Lumina;

namespace LuminaSharp;

/// Agent steering over SPathFollowComponent. The queries themselves live on CNavigationLibrary, and an agent also needs a character-controller component for the movement the path-follow system writes to take effect.
public static class Navigation
{
    /// Upper bound on the corner count a single path query returns.
    public const int MaxPathCorners = 64;

    /// Finds a path and returns it, or null when there is none. Use CNavigationLibrary.FindPath with your own span for a per-frame query that must not allocate.
    public static NavPath? FindPath(CWorld World, FVector3 Start, FVector3 End)
    {
        return FindPath(World, Start, End, out _);
    }

    /// Same, but Result says why, including for the null return, so a failure can be logged with a reason.
    public static NavPath? FindPath(CWorld World, FVector3 Start, FVector3 End, out ENavPathResult Result)
    {
        Result = CNavigationLibrary.GetPathResult(World, Start, End, MaxPathCorners);

        System.Span<FVector3> Buffer = stackalloc FVector3[MaxPathCorners];
        int Count = CNavigationLibrary.FindPath(World, Start, End, Buffer);
        if (Count <= 0)
        {
            return null;
        }

        FVector3[] Corners = new FVector3[Count];
        for (int i = 0; i < Count; ++i)
        {
            Corners[i] = Buffer[i];
        }
        return new NavPath(Corners, Result);
    }

    /// Reason text for a path result, ready to drop into a log line.
    public static string Describe(ENavPathResult Result) => CNavigationLibrary.DescribePathResult(Result);

    /// Sends the agent to a world location, adding the path-follow component if it has none.
    public static SPathFollowComponent MoveTo(CWorld World, FEntity Agent, FVector3 Destination, float Speed = 1.0f)
    {
        SPathFollowComponent Follow = World.GetOrEmplaceComponent<SPathFollowComponent>(Agent)!;
        Follow.Speed = Speed;
        Follow.SetTargetLocation(Destination);
        return Follow;
    }

    /// Makes the agent chase a target, repathing as the target moves.
    public static SPathFollowComponent Follow(CWorld World, FEntity Agent, FEntity Target, float Speed = 1.0f)
    {
        SPathFollowComponent Follow = World.GetOrEmplaceComponent<SPathFollowComponent>(Agent)!;
        Follow.Speed = Speed;
        Follow.SetTargetEntity(Target);
        return Follow;
    }

    /// Clears the agent's goal and cached path. A no-op when it has none.
    public static void StopMoving(CWorld World, FEntity Agent)
    {
        World.TryGetComponent<SPathFollowComponent>(Agent)?.Stop();
    }
}

/// A navmesh path, so an ordered list of world-space corners from start to goal.
public sealed class NavPath
{
    /// Corner points in order. The first is the start and the last the goal, or the nearest reachable point.
    public readonly FVector3[] Corners;

    /// Why the query ended as it did, so a partial or truncated route says which it was.
    public readonly ENavPathResult Result;

    public NavPath(FVector3[] Corners, ENavPathResult Result)
    {
        this.Corners = Corners;
        this.Result = Result;
    }

    /// True when the route stops short of the requested goal, for any reason.
    public bool IsPartial => Result != ENavPathResult.Success;

    /// Reason text for this path's result, ready to drop into a log line.
    public string Reason => CNavigationLibrary.DescribePathResult(Result);

    public int Count => Corners.Length;

    /// The final corner, so the goal or the nearest reachable point.
    public FVector3 Destination => Corners[Corners.Length - 1];

    /// Summed length of the path in world units.
    public float Length
    {
        get
        {
            float Total = 0.0f;
            for (int i = 1; i < Corners.Length; ++i)
            {
                Total += FVector3.Distance(Corners[i], Corners[i - 1]);
            }
            return Total;
        }
    }
}
