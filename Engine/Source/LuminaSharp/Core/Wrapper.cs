using System;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;

namespace LuminaSharp;

// A cached fast constructor for a generated wrapper type, so a native pointer becomes its typed wrapper without reflection.
internal static class Wrapper<T> where T : class
{
    public static readonly Func<IntPtr, T?> Create = Build();

    // The canonical wrapper, cached by a weak handle so identity holds and nothing pins the script context, each hand-out holding a reference.
    public static T? ForObject(IntPtr Pointer)
    {
        if (Pointer == IntPtr.Zero)
        {
            return null;
        }

        IntPtr Existing = Native.ObjectGetManagedInstance(Pointer);
        if (Existing != IntPtr.Zero)
        {
            // Null after a collection or an unloaded context, and a mismatch when it was wrapped as another type.
            if (GCHandle.FromIntPtr(Existing).Target is T Cached)
            {
                if (Cached is NativeObject Object)
                {
                    Object.Acquire(Pointer);
                }
                return Cached;
            }
        }

        T? Instance = Create(Pointer);
        if (Instance == null)
        {
            return null;
        }
        (Instance as NativeObject)?.StartCountingReferences();

        // Set frees the handle it replaces, so the stale one above is not leaked.
        GCHandle Weak = GCHandle.Alloc(Instance, GCHandleType.Weak);
        Native.ObjectSetManagedInstance(Pointer, GCHandle.ToIntPtr(Weak));
        return Instance;
    }

    private static Func<IntPtr, T?> Build()
    {
        ConstructorInfo? Constructor = typeof(T).GetConstructor(
            BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public,
            null, new[] { typeof(IntPtr) }, null);
        if (Constructor == null)
        {
            return Handle => null;
        }

        ParameterExpression Parameter = Expression.Parameter(typeof(IntPtr), "handle");
        return Expression.Lambda<Func<IntPtr, T?>>(Expression.New(Constructor, Parameter), Parameter).Compile();
    }
}
