using System;
using LuminaSharp;

namespace Lumina;

// Typed access to an animation graph's parameter block, which the graph asset names as a reflected struct.
public readonly unsafe partial struct SAnimationGraphComponent
{
    // Goes through the native accessor rather than Parameters, since it initializes the block from the graph first.
    public T GetParameters<T>() where T : struct, INativeStruct<T>
        => TryGetParameters<T>() ?? throw new InvalidOperationException($"Animation graph parameter block is not a {T.NativeName}.");

    public T? TryGetParameters<T>() where T : struct, INativeStruct<T>
        => NativeStructs.FromAddressOrNull<T>(GetParameterMemoryAs(T.NativeName));
}
