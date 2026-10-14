using System;
using System.Runtime.CompilerServices;

namespace LuminaSharp;

// The native memory a struct handle views. One fetched from an entity tracks its pool, so a kept handle survives the pool moving.
public readonly unsafe struct NativeRef : IEquatable<NativeRef>
{
    private const uint DeadEpoch = 0xFFFFFFFFu;

    private readonly nint Base;
    private readonly uint* EpochSlot;
    private readonly ulong WorldHandle;
    private readonly nint Ops;
    private readonly uint Epoch;
    private readonly uint EntityId;
    private readonly int Offset;

    public NativeRef(nint Address)
    {
        Base = Address;
        EpochSlot = null;
        WorldHandle = 0;
        Ops = 0;
        Epoch = 0;
        EntityId = 0;
        Offset = 0;
    }

    internal NativeRef(nint Address, ulong World, uint Entity, nint ComponentOps, uint* Slot)
    {
        Base = Address;
        EpochSlot = Slot;
        WorldHandle = World;
        Ops = ComponentOps;
        Epoch = Slot != null ? *Slot : 0;
        EntityId = Entity;
        Offset = 0;
    }

    private NativeRef(in NativeRef Owner, int Delta)
    {
        Base = Owner.Base;
        EpochSlot = Owner.EpochSlot;
        WorldHandle = Owner.WorldHandle;
        Ops = Owner.Ops;
        Epoch = Owner.Epoch;
        EntityId = Owner.EntityId;
        Offset = Owner.Offset + Delta;
    }

    public bool IsNull => Base == 0;

    // False once the component this views was removed or its world destroyed, or for an empty handle.
    public bool IsValid => EpochSlot == null ? Base != 0 : *EpochSlot == Epoch || Resolve(false) != 0;

    public bool IsTracked => EpochSlot != null;

    public ulong World => WorldHandle;

    public FEntity Entity => new(EntityId);

    public nint Address
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get
        {
            if (EpochSlot == null)
            {
                if (Base == 0)
                {
                    ThrowEmpty();
                }
                return Base + Offset;
            }
            return (*EpochSlot == Epoch ? Base : Resolve(true)) + Offset;
        }
    }

    // A member of the viewed memory, still tracked through the same pool.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public NativeRef Field(int FieldOffset) => new(this, FieldOffset);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void* At(int FieldOffset) => (void*)(Address + FieldOffset);

    // A handle kept past a pool move resolves again on every access, since a readonly copy cannot cache the new address.
    [MethodImpl(MethodImplOptions.NoInlining)]
    private nint Resolve(bool bThrow)
    {
        nint Pointer = 0;
        if (*EpochSlot != DeadEpoch)
        {
            uint* Slot;
            Pointer = Native.GetComponentTracked(WorldHandle, EntityId, Ops, &Slot);
        }

        if (Pointer == 0 && bThrow)
        {
            throw new InvalidOperationException($"The component is no longer on entity {EntityId & 0xFFFFF}; it was removed or its world was destroyed.");
        }
        return Pointer;
    }

    private static void ThrowEmpty()
    {
        throw new NullReferenceException("The struct handle views no native memory.");
    }

    public bool Equals(NativeRef Other) => Base == Other.Base && Offset == Other.Offset && EntityId == Other.EntityId && WorldHandle == Other.WorldHandle;

    public override bool Equals(object? Other) => Other is NativeRef Ref && Equals(Ref);

    public override int GetHashCode() => HashCode.Combine(Base, Offset, EntityId, WorldHandle);
}

// A generated or script struct handle, whose one field is the NativeRef it views.
public interface INativeStruct
{
    NativeRef Ref { get; }
}

public interface INativeStruct<TSelf> : INativeStruct where TSelf : struct, INativeStruct<TSelf>
{
    // The registered name of the native struct, the identity every lookup across the boundary uses.
    static abstract string NativeName { get; }
}

// A struct the ECS stores per entity, reached through the world rather than built in C#.
public interface IComponent<TSelf> : INativeStruct<TSelf> where TSelf : struct, IComponent<TSelf>
{
}

public static unsafe class NativeStructs
{
    // Every handle is exactly one NativeRef, so building one is a reinterpretation rather than a constructor call.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T FromRef<T>(NativeRef Ref) where T : struct, INativeStruct
    {
        Layout<T>.Check();
        return Unsafe.As<NativeRef, T>(ref Ref);
    }

    public static T FromAddress<T>(nint Address) where T : struct, INativeStruct => FromRef<T>(new NativeRef(Address));

    public static T? FromAddressOrNull<T>(nint Address) where T : struct, INativeStruct
    {
        return Address == 0 ? null : FromRef<T>(new NativeRef(Address));
    }

    // A component handed back by pointer tracks its owner like a registry fetch, so a kept handle never reads a moved slot.
    public static T? FromComponentPointer<T>(nint Pointer) where T : struct, IComponent<T>
    {
        if (Pointer == 0)
        {
            return null;
        }

        nint Ops = ComponentOps<T>.Token;
        if (Ops != 0 && Native.TrackComponentPointer(Pointer, Ops, out ulong World, out uint Entity, out uint* Epoch))
        {
            return FromRef<T>(new NativeRef(Pointer, World, Entity, Ops, Epoch));
        }
        return FromRef<T>(new NativeRef(Pointer));
    }

    public static nint AddressOf<T>(T? Value) where T : struct, INativeStruct
    {
        return Value.HasValue && !Value.Value.Ref.IsNull ? Value.Value.Ref.Address : 0;
    }

    public static nint AddressOf<T>(T Value) where T : struct, INativeStruct
    {
        return Value.Ref.IsNull ? 0 : Value.Ref.Address;
    }

    private static class Layout<T> where T : struct
    {
        private static readonly bool bValid = Unsafe.SizeOf<T>() == Unsafe.SizeOf<NativeRef>();

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Check()
        {
            if (!bValid)
            {
                throw new InvalidOperationException($"{typeof(T).Name} is not a struct handle; its only field must be a NativeRef.");
            }
        }
    }
}

// Names the native struct a [Reflect] script struct extends, the closest a C# struct gets to inheriting one.
public interface IExtends<TBase> where TBase : struct, INativeStruct<TBase>
{
}
