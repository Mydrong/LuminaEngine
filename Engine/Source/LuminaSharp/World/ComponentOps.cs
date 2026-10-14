using System;

namespace LuminaSharp;

internal static class ComponentOps<T> where T : struct, IComponent<T>
{
    public static readonly IntPtr Token = Native.FindComponentOps(T.NativeName);
}
