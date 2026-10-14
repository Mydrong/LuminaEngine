namespace Lumina;

// Handwritten ergonomics over the reflected SAudioHandle.
public partial struct SAudioHandle
{
    // False for a sound that never started, whether from missing data, no device, or the voice cap.
    public bool IsValid => Generation != 0;
}

// Handwritten ergonomics over the reflected SAudioPlayParams.
public partial struct SAudioPlayParams
{
    // Reads the engine's own defaults rather than restating them, so the two cannot drift.
    public static SAudioPlayParams Default() => CAudioLibrary.DefaultPlayParams();
}
