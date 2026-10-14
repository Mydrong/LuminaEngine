using LuminaSharp;

namespace Lumina;

// One stage scheduled worker over a world's component store, instanced once per world.
public partial class CEntitySystem
{
    // Valid from Configure onwards, read from the native object that owns the value for both languages.
    public CWorld World => GetWorld();

    // The tick context for the stage currently running, only meaningful inside the callbacks.
    public SystemContext Context => new(Native.WorldGetSystemContext(World.WorldHandle));

    // Schedules this system in Stage at the default priority.
    protected void RequireUpdate(EUpdateStage Stage) => RequireUpdate(Stage, (int)EUpdatePriority.Default);

    // Schedules this system in Stage. Lower priority runs first, so Highest is 0 and Low is 192.
    protected void RequireUpdate(EUpdateStage Stage, EUpdatePriority Priority) => RequireUpdate(Stage, (int)Priority);

    // Declares that this system writes T, so it serializes against anything else reading or writing T.
    protected void Writes<T>() => DeclareWrite(typeof(T).Name);

    // Declares that this system only reads T, so it runs concurrently with any other reader of T.
    protected void Reads<T>() => DeclareRead(typeof(T).Name);
}
