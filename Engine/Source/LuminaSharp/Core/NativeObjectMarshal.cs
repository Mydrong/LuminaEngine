using System;

namespace LuminaSharp;

// Object pointer to wrapper and back for the accessors the rewriter emits, which a user assembly cannot do through internal Wrapper<T>.
public static class NativeObjectMarshal
{
    // The canonical wrapper, so reading a reference twice returns the same instance.
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
    public static T? FromHandle<T>(IntPtr Object) where T : NativeObject
    {
        return Object == IntPtr.Zero ? null : Wrapper<T>.ForObject(Object);
    }

    // The same for a type only known at runtime, its generic instantiation cached.
    private static readonly global::System.Collections.Generic.Dictionary<Type, Func<IntPtr, NativeObject?>> FromHandleByType = new();

    // Dropped on reload, since a cached user type would pin the context being unloaded.
    internal static void ClearTypeCache()
    {
        lock (FromHandleByType)
        {
            FromHandleByType.Clear();
        }
    }

    /// <summary>The canonical wrapper, for a type resolved at runtime rather than named in source.</summary>
    public static NativeObject? FromHandleOfType(IntPtr Object, Type Wanted)
    {
        if (Object == IntPtr.Zero)
        {
            return null;
        }

        Func<IntPtr, NativeObject?>? Make;
        lock (FromHandleByType)
        {
            if (!FromHandleByType.TryGetValue(Wanted, out Make))
            {
                // A delegate rather than the MethodInfo, since invoking one cost an object[] and a box per call.
                Make = (Func<IntPtr, NativeObject?>)typeof(NativeObjectMarshal)
                    .GetMethod(nameof(FromHandle))!
                    .MakeGenericMethod(Wanted)
                    .CreateDelegate(typeof(Func<IntPtr, NativeObject?>));
                FromHandleByType[Wanted] = Make;
            }
        }

        return Make(Object);
    }

    /// <summary>The native pointer behind a wrapper, or zero for null.</summary>
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
    public static IntPtr ToHandle(NativeObject? Value)
    {
        return Value is null ? IntPtr.Zero : Value.Handle;
    }
}
