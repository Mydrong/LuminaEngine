using System;
using System.Runtime.CompilerServices;

namespace LuminaSharp;

// A delegate listener, with the broadcast's argument pack as the payload.
internal abstract class DelegateInvoker : ScriptClosure
{
    protected abstract void Run(IntPtr Payload);

    public sealed override ulong Invoke(ulong Payload)
    {
        Run((IntPtr)Payload);
        return 0;
    }
}

internal sealed class VoidInvoker : DelegateInvoker
{
    public Action Handler = null!;

    protected override void Run(IntPtr Payload) => Handler();
}

internal sealed class PayloadInvoker<T> : DelegateInvoker where T : unmanaged
{
    public Action<T> Handler = null!;

    protected override unsafe void Run(IntPtr Payload) => Handler(Unsafe.Read<T>((void*)Payload));
}

// The delegate owns the callback from the moment it is bound and releases it when the binding retires.
internal static unsafe class DelegateBindings
{
    internal static DelegateBinding Bind(void* Address, DelegateInvoker Invoker)
    {
        if (Address == null)
        {
            return default;
        }

        ulong Id = Native.DelegateBind((IntPtr)Address, ScriptCallback.Bind(Invoker).Token);
        return Id == 0 ? default : new DelegateBinding((IntPtr)Address, Id);
    }
}
