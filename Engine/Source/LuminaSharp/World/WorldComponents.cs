using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using LuminaSharp;

namespace Lumina;

public unsafe partial class CWorld
{
    // Null when there is none, like FRegistry::TryGet, and tracked through the pool so one kept in a field stays valid when the pool moves.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public TComponentRef<T>? TryGetComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        uint* Epoch = null;
        ulong World = WorldHandle;
        IntPtr Pointer = Native.GetComponentTracked(World, Entity.Id, ComponentOps<T>.Token, &Epoch);
        return Pointer == IntPtr.Zero ? null : new TComponentRef<T>(Pointer, World, Entity.Id, Epoch);
    }

    // In place like FRegistry::Get, so the ref is only good until the next structural change to this type.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public ref T GetComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        IntPtr Pointer = Native.GetComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token);
        if (Pointer == IntPtr.Zero)
        {
            ThrowMissingComponent(Entity, T.NativeName);
        }
        return ref Unsafe.AsRef<T>((void*)Pointer);
    }

    [System.Diagnostics.CodeAnalysis.DoesNotReturn]
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void ThrowMissingComponent(FEntity Entity, string Component)
    {
        throw new InvalidOperationException($"Entity {Entity.Id} has no {Component}.");
    }

    // A tag has no storage, so its ref is null and only good for discarding.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public ref T GetOrEmplaceComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        IntPtr Pointer = Native.EmplaceComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token);
        return ref Pointer == IntPtr.Zero ? ref Unsafe.NullRef<T>() : ref Unsafe.AsRef<T>((void*)Pointer);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool HasComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        return Native.HasComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token) != 0;
    }

    public bool RemoveComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        return Native.RemoveComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token) != 0;
    }

    public RegistrySubscription OnConstruct<T>(Action<FEntity> Callback) where T : struct, IComponent<T>
        => Subscribe(ComponentOps<T>.Token, ERegistrySignal.Construct, Callback);

    public RegistrySubscription OnDestroy<T>(Action<FEntity> Callback) where T : struct, IComponent<T>
        => Subscribe(ComponentOps<T>.Token, ERegistrySignal.Destroy, Callback);

    public RegistrySubscription OnUpdate<T>(Action<FEntity> Callback) where T : struct, IComponent<T>
        => Subscribe(ComponentOps<T>.Token, ERegistrySignal.Update, Callback);

    public void PatchComponent<T>(FEntity Entity) where T : struct, IComponent<T>
        => Native.RegistryPatch(WorldHandle, Entity.Id, ComponentOps<T>.Token);

    private RegistrySubscription Subscribe(IntPtr Token, ERegistrySignal Signal, Action<FEntity> Callback)
    {
        if (Token == IntPtr.Zero)
        {
            return RegistrySubscription.Empty;
        }

        int Kind = (int)Signal;
        IntPtr Listener = Native.RegistryConnect(WorldHandle, Token, Kind);
        if (Listener == IntPtr.Zero)
        {
            return RegistrySubscription.Empty;
        }
        IntPtr SignalDelegate = Native.RegistryGetSignalDelegate(Listener);
        DelegateBinding Binding;
        unsafe
        {
            Binding = DelegateBindings.Bind((void*)SignalDelegate,
                new PayloadInvoker<uint> { Handler = Id => Callback(new FEntity(Id)) });
        }

        if (!Binding.IsValid)
        {
            Native.RegistryDisconnect(WorldHandle, Token, Kind, Listener);
            return RegistrySubscription.Empty;
        }

        return new RegistrySubscription(WorldHandle, Token, Kind, Listener, Binding);
    }

    private static TSubclassOf<CEntityScript> ClassOf<T>() where T : CEntityScript => NativeClass<T>.Of.AsBase<CEntityScript>();
    private static TSubclassOf<CEntityScript> ScriptClassOf<T>() where T : class => ScriptClass<T>.Of;

    // The class per script type, kept like NativeClass<T> so a miss resolves again once the class is minted.
    private static class ScriptClass<T> where T : class
    {
        private static TSubclassOf<CEntityScript> Resolved;

        public static TSubclassOf<CEntityScript> Of
        {
            get
            {
                if (!Resolved.IsValid && typeof(CEntityScript).IsAssignableFrom(typeof(T)))
                {
                    Resolved = TSubclassOf<CEntityScript>.FromName(NativeTypeName.Of<T>());
                }
                return Resolved;
            }
        }
    }

    public T? GetScript<T>(FEntity Entity) where T : class
    {
        if (!typeof(T).IsInterface && !typeof(CEntityScript).IsAssignableFrom(typeof(T)))
        {
            return null;
        }

        TSubclassOf<CEntityScript> ScriptClass = ScriptClassOf<T>();
        if (!ScriptClass.IsValid)
        {
            foreach (CEntityScript Script in FindScriptsOfAbstract(Entity))
            {
                if (AsScript<T>(Script) is T Typed)
                {
                    return Typed;
                }
            }
            return null;
        }
        return AsScript<T>(FindEntityScript(Entity, ScriptClass));
    }

    private CEntityScript[] FindScriptsOfAbstract(FEntity Entity)
    {
        return FindEntityScripts(Entity, ClassOf<CEntityScript>());
    }

    private static T? AsScript<T>(CEntityScript? Script) where T : class
    {
        if (Script is T Typed)
        {
            return Typed;
        }
        return Script is null ? null : Wrapper<T>.ForObject(Script.Handle);
    }

    public List<T> GetScripts<T>(FEntity Entity) where T : class
    {
        if (!typeof(T).IsInterface && !typeof(CEntityScript).IsAssignableFrom(typeof(T)))
        {
            return new List<T>();
        }

        TSubclassOf<CEntityScript> ScriptClass = ScriptClassOf<T>();
        CEntityScript[] Scripts = ScriptClass.IsValid
            ? FindEntityScripts(Entity, ScriptClass)
            : FindScriptsOfAbstract(Entity);
        var Result = new List<T>(Scripts.Length);
        foreach (CEntityScript Script in Scripts)
        {
            if (AsScript<T>(Script) is T Typed)
            {
                Result.Add(Typed);
            }
        }
        return Result;
    }

    public T? AddScript<T>(FEntity Entity) where T : CEntityScript
    {
        return AsScript<T>(AddEntityScript(Entity, ClassOf<T>()));
    }

    public bool RemoveScript<T>(FEntity Entity) where T : class
    {
        CEntityScript? Script = GetScript<T>(Entity) as CEntityScript;
        return Script is not null && RemoveEntityScript(Entity, Script);
    }

    public bool HasScript<T>(FEntity Entity) where T : class
    {
        return GetScript<T>(Entity) != null;
    }

    public View<T1> View<T1>(Exclude Filter = default) where T1 : struct, IComponent<T1>
        => new(WorldHandle, Filter);

    public View<T1, T2> View<T1, T2>(Exclude Filter = default) where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2>
        => new(WorldHandle, Filter);

    public View<T1, T2, T3> View<T1, T2, T3>(Exclude Filter = default)
        where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3>
        => new(WorldHandle, Filter);

    public View<T1, T2, T3, T4> View<T1, T2, T3, T4>(Exclude Filter = default)
        where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4>
        => new(WorldHandle, Filter);

    public View<T1, T2, T3, T4, T5> View<T1, T2, T3, T4, T5>(Exclude Filter = default)
        where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4> where T5 : struct, IComponent<T5>
        => new(WorldHandle, Filter);

    public View<T1, T2, T3, T4, T5, T6> View<T1, T2, T3, T4, T5, T6>(Exclude Filter = default)
        where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4> where T5 : struct, IComponent<T5> where T6 : struct, IComponent<T6>
        => new(WorldHandle, Filter);

    public View<T1, T2, T3, T4, T5, T6, T7> View<T1, T2, T3, T4, T5, T6, T7>(Exclude Filter = default)
        where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4> where T5 : struct, IComponent<T5> where T6 : struct, IComponent<T6> where T7 : struct, IComponent<T7>
        => new(WorldHandle, Filter);

    public View<T1, T2, T3, T4, T5, T6, T7, T8> View<T1, T2, T3, T4, T5, T6, T7, T8>(Exclude Filter = default)
        where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4> where T5 : struct, IComponent<T5> where T6 : struct, IComponent<T6> where T7 : struct, IComponent<T7> where T8 : struct, IComponent<T8>
        => new(WorldHandle, Filter);
}

// Which registry signal a subscription listens to, in the order the native connect export numbers them.
internal enum ERegistrySignal
{
    Construct,
    Destroy,
    Update,
}
