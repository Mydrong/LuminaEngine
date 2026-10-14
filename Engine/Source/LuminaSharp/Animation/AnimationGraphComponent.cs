using System;
using LuminaSharp;

namespace Lumina;

// Typed access to an animation graph's parameter block, which the graph asset names as a reflected struct.
public readonly unsafe partial struct SAnimationGraphComponent
{
    // Named for the C++ accessor, since the reflected Parameters field now binds under its own name.
    public T? GetParameters<T>() where T : struct, INativeStruct<T>
    {
        return NativeStructs.FromAddressOrNull<T>(GetParameterMemoryAs(T.NativeName));
    }

    public T RequireParameters<T>() where T : struct, INativeStruct<T>
    {
        return GetParameters<T>() ?? throw new InvalidOperationException(
            $"Animation graph parameter block is not a {T.NativeName}.");
    }
}
