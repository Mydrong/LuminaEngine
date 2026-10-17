using System;
using LuminaSharp;

namespace Lumina;

// The C# half of the context a system's callbacks receive, so a C# system drains and sends events the way a C++ one does.
public readonly unsafe partial struct SSystemContext
{
    // The native struct leads with the world pointer, which EntitySystem.cpp asserts.
    public CWorld World => NativeObjectMarshal.FromHandle<CWorld>(*(IntPtr*)Self)!;

    // Readable after this batch's sync point, given SendsEvent<T> was declared.
    public void SendEvent<T>(in T Event) where T : unmanaged, IEvent => World.Events.Send(in Event);

    public void SendEvents<T>(ReadOnlySpan<T> Events) where T : unmanaged, IEvent => World.Events.Send(Events);

    // Every T sent since Reader last read, oldest first, for a Reader that ReadsEvent<T> handed out.
    public EventSpan<T> ReadEvents<T>(ref EventReader<T> Reader) where T : unmanaged, IEvent => World.Events.Read(ref Reader);

    // ReadEvents for an event that holds strings or object references, each one a view over the queued value.
    public EventViews<T> ReadEventViews<T>(ref EventReader<T> Reader) where T : struct, INativeStruct<T>, IEvent => World.Events.ReadViews(ref Reader);

    // For HasWork, so a system that only answers events stays out of a frame that sent none.
    public bool HasUnreadEvents<T>(in EventReader<T> Reader) where T : struct, IEvent => World.Events.HasUnread(in Reader);
}
