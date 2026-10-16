namespace LuminaSharp;

/// <summary>
/// Lightweight gameplay CPU profiler. Wrap any gameplay work in a scope and it shows up,  aggregated by
/// name, with call count and inclusive/self ms,  in the editor's Gameplay Profiler tool. Near-zero cost
/// when nobody is recording (<see cref="Enabled"/> is false). Game thread only.
///
/// Entity script and system <c>OnUpdate</c> are auto-profiled by their type name, so per-script timings
/// appear with no work. Use <see cref="Sample"/> to break a hot method into sub-scopes:
/// <code>using (Profiler.Sample("Perception")) { ... }</code>
/// </summary>
public static class Profiler
{
    /// <summary>True while the editor Gameplay Profiler is open (or recording is otherwise enabled).</summary>
    public static bool Enabled => Lumina.CGameplayProfilerLibrary.IsProfilerEnabled();

    /// <summary>Open a named scope. Pair with <see cref="End"/>; prefer <see cref="Sample"/> for exception safety.</summary>
    public static void Begin(string Name) => Lumina.CGameplayProfilerLibrary.BeginRegisteredScope(ScopeId(Name));

    // Names are registered natively once, after which a sample sends only an id. Concurrent, since parallel OnUpdate samples too.
    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, int> ScopeIds = new();

    private static int ScopeId(string Name)
    {
        return ScopeIds.GetOrAdd(Name, static Key => Lumina.CGameplayProfilerLibrary.RegisterScope(Key));
    }

    /// <summary>Close the most recently opened scope.</summary>
    public static void End() => Lumina.CGameplayProfilerLibrary.EndScope();

    /// <summary>
    /// A <c>using</c>-scoped timer that closes when the block exits (including on exceptions):
    /// <c>using (Profiler.Sample("Pathfind")) { ... }</c>.
    /// </summary>
    public static Scope Sample(string Name) => new Scope(Name);

    /// <summary>The disposable returned by <see cref="Sample"/>. Stack-only (a ref struct); do not store it.</summary>
    public readonly ref struct Scope
    {
        // Decided once at the open, so the close always matches whatever the open did.
        private readonly bool bOpen;

        public Scope(string Name)
        {
            bOpen = Native.IsProfilingScopes;
            if (bOpen)
            {
                Lumina.CGameplayProfilerLibrary.BeginRegisteredScope(ScopeId(Name));
            }
        }

        public void Dispose()
        {
            if (bOpen)
            {
                Lumina.CGameplayProfilerLibrary.EndScope();
            }
        }
    }
}
