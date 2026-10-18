using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Lumina;

namespace LuminaSharp;

// Every way native calls back into managed code is one of these behind a GCHandle, carried as an SScriptCallback.
public abstract class ScriptClosure
{
    // One rule for every kind of callback, it runs in the script context it was bound in.
    private readonly Engine.Scope BoundIn = Engine.Snapshot();

    // The payload's meaning is the binding's own; the answer goes back to native, which most callers ignore.
    public abstract ulong Invoke(ulong Payload);

    internal ulong Run(ulong Payload)
    {
        using Engine.Scope Prior = Engine.Resume(BoundIn);
        return Invoke(Payload);
    }
}

// Holds the caller's own delegate, so binding one allocates no adapter around it.
internal sealed class ActionClosure : ScriptClosure
{
    private readonly Delegate Handler;

    public ActionClosure(Delegate Handler)
    {
        this.Handler = Handler;
    }

    public override ulong Invoke(ulong Payload)
    {
        switch (Handler)
        {
            case Action Plain:
                Plain();
                break;
            case Action<FEntity> OnEntity:
                OnEntity(new FEntity((uint)Payload));
                break;
            case Action<float> OnFloat:
                OnFloat(BitConverter.Int32BitsToSingle(unchecked((int)(uint)Payload)));
                break;
            case Action<int> OnInt:
                OnInt(unchecked((int)(uint)Payload));
                break;
        }
        return 0;
    }
}

// Builds the SScriptCallback a reflected native function takes. Native decides whether it fires once or repeatedly.
public static class ScriptCallback
{
    // Tracked so a generation unload can clear closures over user types before they pin the collectible context.
    private static readonly HashSet<IntPtr> Live = new();

    public static SScriptCallback Of(Action Handler)
    {
        return Bind(new ActionClosure(Handler));
    }

    // The payload is the entity the operation produced, or the null entity when it produced none.
    public static SScriptCallback Of(Action<FEntity> Handler)
    {
        return Bind(new ActionClosure(Handler));
    }

    // The payload is a float packed by Scripting::PackFloatPayload, such as a tween's value.
    public static SScriptCallback OfFloat(Action<float> Handler)
    {
        return Bind(new ActionClosure(Handler));
    }

    // The payload is a count or an index, such as a tween's completed loops.
    public static SScriptCallback OfInt(Action<int> Handler)
    {
        return Bind(new ActionClosure(Handler));
    }

    public static SScriptCallback Bind(ScriptClosure Closure)
    {
        IntPtr Token = GCHandle.ToIntPtr(GCHandle.Alloc(Closure));
        lock (Live)
        {
            Live.Add(Token);
        }
        return new SScriptCallback { Token = (ulong)Token };
    }

    // Runs the closure and keeps it bound; native frees it through FreeHandle when it is done with it.
    [ManagedExport]
    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
    public static ulong InvokeScriptCallback(IntPtr Token, ulong Payload)
    {
        try
        {
            return Token != IntPtr.Zero && GCHandle.FromIntPtr(Token).Target is ScriptClosure Closure ? Closure.Run(Payload) : 0;
        }
        catch (Exception Exception)
        {
            Interop.LogException(Exception);
            return 0;
        }
    }

    // Frees any handle native was given, a callback, a message payload or an object twin, since native never needs to know which.
    [ManagedExport]
    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
    public static void FreeHandle(IntPtr Handle)
    {
        try
        {
            if (Handle == IntPtr.Zero)
            {
                return;
            }
            lock (Live)
            {
                Live.Remove(Handle);
            }
            // Before the free, so the live set can never hold a value the runtime may recycle.
            Host.ForgetScriptHandle(Handle);
            GCHandle.FromIntPtr(Handle).Free();
        }
        catch (Exception Exception)
        {
            Interop.LogException(Exception);
        }
    }

    // Lets the native instance table tell a collected weak twin from a live one.
    [ManagedExport]
    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
    public static int IsHandleAlive(IntPtr Handle)
    {
        try
        {
            return Handle != IntPtr.Zero && GCHandle.FromIntPtr(Handle).Target != null ? 1 : 0;
        }
        catch (Exception Exception)
        {
            Interop.LogException(Exception);
            return 0;
        }
    }

    // Cleared but left allocated, so a later native release still frees the handle and finds nothing to run.
    internal static void PurgeAll()
    {
        lock (Live)
        {
            foreach (IntPtr Token in Live)
            {
                GCHandle Handle = GCHandle.FromIntPtr(Token);
                if (Handle.IsAllocated)
                {
                    Handle.Target = null;
                }
            }
            Live.Clear();
        }
    }
}
