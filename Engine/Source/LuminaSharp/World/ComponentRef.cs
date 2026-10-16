using System;
using System.Runtime.CompilerServices;

namespace LuminaSharp;

// One entity's component, found again after its pool moves, laid out as C++ FComponentRef so a stored one reads in place.
public readonly unsafe struct TComponentRef<T> : IEquatable<TComponentRef<T>> where T : struct, IComponent<T>
{
    private readonly NativeRef Tracked;

    internal TComponentRef(NativeRef Tracked)
    {
        this.Tracked = Tracked;
    }

    internal TComponentRef(nint Pointer, ulong World, uint Entity, uint* EpochSlot)
        : this(new NativeRef(Pointer, World, Entity, ComponentOps<T>.Token, EpochSlot))
    {
    }

    // False when unbound, once the component is removed, or once its world is destroyed.
    public bool IsValid
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => Tracked.IsValid;
    }

    public FEntity Entity => Tracked.Entity;

    // The component in its pool, re-found first when the pool has moved, and a throw when it is gone.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public ref T Get() => ref Unsafe.AsRef<T>((void*)Tracked.Address);

    // The tracked reference to a component already reached in place, such as the ref GetOrEmplaceComponent returns.
    public static TComponentRef<T> Of(ref T Component) => FromPointer((nint)Unsafe.AsPointer(ref Component));

    // A pointer a native call handed back, tracked through the pool it belongs to when native can name one.
    public static TComponentRef<T> FromPointer(nint Pointer)
    {
        if (Pointer == 0)
        {
            return default;
        }

        nint Ops = ComponentOps<T>.Token;
        if (Ops != 0 && Native.TrackComponentPointer(Pointer, Ops, out ulong World, out uint Entity, out uint* Epoch))
        {
            return new TComponentRef<T>(new NativeRef(Pointer, World, Entity, Ops, Epoch));
        }
        return new TComponentRef<T>(new NativeRef(Pointer));
    }

    public bool Equals(TComponentRef<T> Other) => Tracked.Equals(Other.Tracked);

    public override bool Equals(object? Other) => Other is TComponentRef<T> Ref && Equals(Ref);

    public override int GetHashCode() => Tracked.GetHashCode();

    public static bool operator ==(TComponentRef<T> A, TComponentRef<T> B) => A.Equals(B);

    public static bool operator !=(TComponentRef<T> A, TComponentRef<T> B) => !A.Equals(B);
}
