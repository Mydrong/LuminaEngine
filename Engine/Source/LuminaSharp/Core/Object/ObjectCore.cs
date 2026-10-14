using System;
using Lumina;

namespace LuminaSharp;

// The script half of ObjectCore.h, imported into every script with a global using static so calls read as they do in C++.
public static class ObjectCore
{
    // Blocking load by virtual path, as LoadObject<T> in C++; the wrapper holds a strong reference like a TStrongObjectPtr member.
    public static T? LoadObject<T>(string Path) where T : NativeObject => Wrapper<T>.ForObject((IntPtr)CAssetLibrary.LoadSynchronous(Path));

    // Loads through FAssetManager, and the callback runs once on the game thread with the object or null, which it holds from then on.
    public static void AsyncLoadObject<T>(string Path, Action<T?> Callback) where T : NativeObject
    {
        FAssetHandle Handle = FAssetManager.Load(Path);
        Handle.OnReady(Loaded =>
        {
            Callback(Loaded.Get<T>());
            Loaded.Dispose();
        });
    }
}
