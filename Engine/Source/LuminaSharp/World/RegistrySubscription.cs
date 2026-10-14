using System;

namespace LuminaSharp;

public sealed class RegistrySubscription : IDisposable
{
    /// <summary>An inert subscription (returned when the component type is unknown or the connect failed).</summary>
    internal static readonly RegistrySubscription Empty = new();

    private readonly ulong WorldHandle;
    private readonly IntPtr Token;
    private readonly int Kind;
    private IntPtr Listener;
    private DelegateBinding Binding;

    private RegistrySubscription()
    {
    }

    internal RegistrySubscription(ulong WorldHandle, IntPtr Token, int Kind, IntPtr Listener, DelegateBinding Binding)
    {
        this.WorldHandle = WorldHandle;
        this.Token = Token;
        this.Kind = Kind;
        this.Listener = Listener;
        this.Binding = Binding;
    }

    /// <summary>True while connected; false for an inert/failed subscription or after <see cref="Dispose"/>.</summary>
    public bool IsActive => Listener != IntPtr.Zero;

    public void Dispose()
    {
        if (Listener == IntPtr.Zero)
        {
            return;
        }

        // Unbind first: the listener owns the delegate, so after it goes the binding has nothing to name.
        Binding.Dispose();
        Binding = default;

        Native.RegistryDisconnect(WorldHandle, Token, Kind, Listener);
        Listener = IntPtr.Zero;
    }
}
