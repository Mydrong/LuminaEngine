using System;

namespace LuminaSharp;

internal static class ComponentOps<T> where T : NativeStruct
{
    public static readonly IntPtr Token = Native.FindComponentOps(typeof(T).Name);

    [ThreadStatic]
    internal static T? ScratchView;
}
