using Lumina;

namespace LuminaSharp;

/// A live camera shake returned by World.Camera.PlayShake. A lightweight value handle, safe to copy. Stopping it fades out over its blend-out time rather than cutting.
public readonly struct FCameraShake
{
    private readonly CWorld World;

    /// Opaque handle. Zero means the shake was not created.
    public readonly uint Id;

    public FCameraShake(CWorld World, uint Id)
    {
        this.World = World;
        this.Id = Id;
    }

    public bool IsValid => Id != 0;

    public void Stop()
    {
        if (Id != 0)
        {
            World.Camera.StopShake(Id);
        }
    }
}
