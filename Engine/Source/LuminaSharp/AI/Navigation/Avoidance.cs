using Lumina;

namespace LuminaSharp;

/// Opt-in local avoidance over SRVOAgentComponent. An agent with one has its path-follow velocity filtered so it steers around other agents instead of walking through them, which the navmesh route alone does not do.
public static class Avoidance
{
    /// Turns on avoidance for the agent, adding the component if it has none. Safe to call on an agent that already has it.
    public static SRVOAgentComponent Enable(CWorld World, FEntity Agent)
    {
        return World.GetOrEmplaceComponent<SRVOAgentComponent>(Agent);
    }

    /// Same, with the two knobs worth setting per agent type. Radius of zero tracks the character capsule.
    public static SRVOAgentComponent Enable(CWorld World, FEntity Agent, float Radius, float TimeHorizon)
    {
        SRVOAgentComponent Avoid = World.GetOrEmplaceComponent<SRVOAgentComponent>(Agent);
        Avoid.Radius = Radius;
        Avoid.TimeHorizon = TimeHorizon;
        return Avoid;
    }

    /// Stops the agent avoiding anyone. Others still avoid it, so a player or a boss keeps its ground.
    public static void SetIgnoresNeighbors(CWorld World, FEntity Agent, bool bIgnore)
    {
        if (World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid)
        {
            Avoid.bIgnoreNeighbors = bIgnore;
        }
    }

    /// Below 0.5 the agent takes less than its half of each mutual correction, so others yield to it.
    public static void SetResponsibility(CWorld World, FEntity Agent, float Share)
    {
        if (World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid)
        {
            Avoid.Responsibility = Share;
        }
    }

    /// Keeps the solved velocity on the navmesh at the cost of one raycast per solve. Off by default.
    public static void SetClampToNavMesh(CWorld World, FEntity Agent, bool bClamp)
    {
        if (World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid)
        {
            Avoid.bClampToNavMesh = bClamp;
        }
    }

    /// True when the agent is currently steering around someone.
    public static bool IsAvoiding(CWorld World, FEntity Agent)
    {
        return World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid && Avoid.LastNeighborCount > 0;
    }

    /// Neighbors that constrained the agent's most recent solve, or zero when it has no avoidance.
    public static int GetNeighborCount(CWorld World, FEntity Agent)
    {
        return World.TryGetComponent<SRVOAgentComponent>(Agent)?.LastNeighborCount ?? 0;
    }

    /// How far avoidance pushed the agent off its intended course, in meters per second. Useful for driving a lean or a sidestep animation.
    public static float GetAvoidanceMagnitude(CWorld World, FEntity Agent)
    {
        return World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid ? Avoid.GetAvoidanceMagnitude() : 0.0f;
    }

    /// The velocity the agent is actually taking this frame, after avoidance.
    public static FVector3 GetVelocity(CWorld World, FEntity Agent)
    {
        return World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid ? Avoid.GetAvoidanceVelocity() : new FVector3(0.0f);
    }

    /// Drives the agent directly, for a flow field or custom steering rather than a navmesh path. Call it every frame; a frame without a call leaves the agent stopped.
    public static void SetPreferredVelocity(CWorld World, FEntity Agent, FVector3 Velocity)
    {
        if (World.TryGetComponent<SRVOAgentComponent>(Agent) is { } Avoid)
        {
            Avoid.SetPreferredVelocity(Velocity);
        }
    }
}
