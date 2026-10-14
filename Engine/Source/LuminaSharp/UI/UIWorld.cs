using System;

namespace LuminaSharp;

internal static class UIWorld
{
    internal static Lumina.CWorld FromHandle(ulong Handle)
        => Wrapper<Lumina.CWorld>.ForObject((IntPtr)Handle)!;
}
