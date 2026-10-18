using System;
using Lumina;

namespace LuminaSharp;

/** The engine's process-wide events, subscribed with += and -= the same way a component event is. */
public static class CoreEvents
{
    /** Fires after a world is torn down, before the next one exists. */
    public static event Action PostWorldUnload { add => Events.PostWorldUnload += value; remove => Events.PostWorldUnload -= value; }

    /** Fires once as the engine begins shutting down, after which listeners are dropped. */
    public static event Action PreEngineShutdown { add => Events.OnPreEngineShutdown += value; remove => Events.OnPreEngineShutdown -= value; }

    /** Fires when gameplay asks to quit; under the editor this ends PIE rather than the process. */
    public static event Action GameQuitRequested { add => Events.OnGameQuitRequested += value; remove => Events.OnGameQuitRequested -= value; }

    /** Fires each frame after the OS event pump, before input actions are evaluated. */
    public static event Action InputPumped { add => Events.OnInputPumped += value; remove => Events.OnInputPumped -= value; }

    private static SCoreDelegates Events => new SCoreDelegates((nint)CCoreDelegateLibrary.GetCoreDelegates());
}
