using System;
using System.Threading;

using Lumina;

namespace LuminaSharp;

// Mirror of C++ FAssetHandle. Keeps what it loaded resident until disposed, and one never disposed lets go when collected.
public sealed class FAssetHandle : IDisposable
{
    private ulong Handle;

    internal FAssetHandle(ulong InHandle)
    {
        Handle = InHandle;
    }

    ~FAssetHandle()
    {
        if (!ObjectReference.HostShutDown)
        {
            Release();
        }
    }

    public bool IsValid => Handle != 0;
    public bool IsReady => CAssetLibrary.IsReady(Handle);

    // Loaded over requested, one once ready.
    public float Progress => CAssetLibrary.GetProgress(Handle);

    // In request order, the assets first and then what their bundle added.
    public int Num => CAssetLibrary.Num(Handle);

    // Null where the load failed or the object is not a T.
    public T? Get<T>(int Index = 0) where T : NativeObject
        => Wrapper<T>.ForObject((IntPtr)CAssetLibrary.Get(Handle, Index, NativeClass<T>.Of.AsBase<NativeObject>()));

    // Runs on the game thread once everything has loaded, and right away when that already happened.
    public void OnReady(Action<FAssetHandle> Callback) => CAssetLibrary.OnReady(Handle, ScriptCallback.Of(() => Callback(this)));

    public System.Runtime.CompilerServices.TaskAwaiter<FAssetHandle> GetAwaiter() => GameTask.WhenReady(this).GetAwaiter();

    public void Dispose()
    {
        Release();
        GC.SuppressFinalize(this);
    }

    private void Release()
    {
        ulong Owned = Interlocked.Exchange(ref Handle, 0);
        if (Owned != 0)
        {
            CAssetLibrary.Release(Owned);
        }
    }
}

// Mirror of C++ FAssetManager.
public static class FAssetManager
{
    // The asset, plus every soft reference its properties put in Bundle when one is named.
    public static FAssetHandle Load(string Path, string Bundle = "") => new(CAssetLibrary.Load(Path, Bundle));

    // A [Reflect(Group = ...)] asset by its group and name, which C++ spells Load(SAssetId).
    public static FAssetHandle LoadByName(string Group, string Name, string Bundle = "") => new(CAssetLibrary.LoadByName(Group, Name, Bundle));

    // Every asset in the group, each with its Bundle when one is named.
    public static FAssetHandle LoadGroup(string Group, string Bundle = "") => new(CAssetLibrary.LoadGroup(Group, Bundle));

    // The asset names in a group, answered from the registry so nothing loads.
    public static string[] GetGroup(string Group) => CAssetLibrary.GetGroupNames(Group).Split('\n', StringSplitOptions.RemoveEmptyEntries);
}
