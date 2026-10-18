using Lumina;

namespace LuminaSharp;

/// Which fields of an EmitParticle call replace what the emitter's spawn stack computed. Mirrors EParticleEmitFlags.
[System.Flags]
public enum EParticleEmitFlags
{
    Position = 1 << 0,
    Velocity = 1 << 1,
    Color    = 1 << 2,
    Size     = 1 << 3,
}

/// Play particle effects in the current world, the visual counterpart to <see cref="Sound"/>.
public static class Fx
{
    /// Despawns a one-shot effect once its last particle dies. A streaming system then lives until the caller destroys it.
    public const float AutoLifetime = -1.0f;

    /// Bursts System at a world point and despawns it after Lifetime seconds. Returns the effect entity.
    public static FEntity Play(CParticleSystem? System, FVector3 Location, float Lifetime = AutoLifetime)
        => Play(System, new FTransform(Location, FQuat.Identity, FVector3.One), Lifetime);

    /// Bursts System oriented along Normal, the shape an impact decal or spark wants.
    public static FEntity PlayAligned(CParticleSystem? System, FVector3 Location, FVector3 Normal, float Lifetime = AutoLifetime)
        => Play(System, new FTransform(Location, FQuat.FromToRotation(FVector3.Up, Normal), FVector3.One), Lifetime);

    /// Bursts System at a full transform, so a scaled or pre-rotated effect keeps its authored orientation.
    public static FEntity Play(CParticleSystem? System, FTransform Transform, float Lifetime = AutoLifetime)
        => System == null ? FEntity.Null : Engine.World.SpawnParticleSystem(System, Transform, Lifetime);

    /// Parents the effect to Target so it follows, optionally on a named socket or bone.
    public static FEntity PlayAttached(CParticleSystem? System, FEntity Target, string Socket = "",
        FVector3 Offset = default, float Lifetime = AutoLifetime)
        => System == null ? FEntity.Null : Engine.World.SpawnParticleSystemAttached(System, Target, Socket, Offset, Lifetime);

    /// Resolves the reference (asset-manager cached) and plays it; a null or unset reference is a no-op.
    public static FEntity Play(TSoftObjectPtr<CParticleSystem> System, FVector3 Location, float Lifetime = AutoLifetime)
        => Play(System.LoadSynchronous(), Location, Lifetime);

    public static FEntity PlayAligned(TSoftObjectPtr<CParticleSystem> System, FVector3 Location, FVector3 Normal, float Lifetime = AutoLifetime)
        => PlayAligned(System.LoadSynchronous(), Location, Normal, Lifetime);

    public static FEntity PlayAttached(TSoftObjectPtr<CParticleSystem> System, FEntity Target, string Socket = "",
        FVector3 Offset = default, float Lifetime = AutoLifetime)
        => PlayAttached(System.LoadSynchronous(), Target, Socket, Offset, Lifetime);

    /// Stops an effect entity emitting and lets its live particles finish, rather than cutting them off.
    public static void Stop(FEntity Effect)
    {
        if (Engine.World.TryGetComponent<SParticleSystemComponent>(Effect) is { } System)
        {
            System.Get().Deactivate();
        }
    }
}
