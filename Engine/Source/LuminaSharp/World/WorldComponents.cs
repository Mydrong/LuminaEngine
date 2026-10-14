using System;
using System.Collections.Generic;
using LuminaSharp;

namespace Lumina;

public unsafe partial class CWorld
{
    // A handle tracked through the component's pool, so one kept in a field stays valid when the pool moves.
    public T? TryGetComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        IntPtr Ops = ComponentOps<T>.Token;
        uint* Epoch = null;
        IntPtr Pointer = Native.GetComponentTracked(WorldHandle, Entity.Id, Ops, &Epoch);
        return Pointer == IntPtr.Zero ? null : NativeStructs.FromRef<T>(new NativeRef(Pointer, WorldHandle, Entity.Id, Ops, Epoch));
    }

    public bool TryGetComponent<T>(FEntity Entity, out T Component) where T : struct, IComponent<T>
    {
        T? Found = TryGetComponent<T>(Entity);
        Component = Found.GetValueOrDefault();
        return Found.HasValue;
    }

    public T GetComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        return TryGetComponent<T>(Entity) ?? throw new InvalidOperationException($"Entity {Entity.Id} has no {T.NativeName}.");
    }

    // A tag has no storage to view, so its handle comes back empty.
    public T GetOrEmplaceComponent<T>(FEntity Entity) where T : struct, IComponent<T>
    {
        Native.EmplaceComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token);
        return TryGetComponent<T>(Entity) ?? default;
    }

    // The pointer is invalid after pool relocation, component removal, or world destruction.
    public IntPtr TryGetComponentPointer<T>(FEntity Entity) where T : struct, IComponent<T>
        => Native.GetComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token);

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
    private static TSubclassOf<CEntityScript> ScriptClassOf<T>() where T : class
    {
        return typeof(CEntityScript).IsAssignableFrom(typeof(T))
            ? TSubclassOf<CEntityScript>.FromName(NativeTypeName.Of(typeof(T)))
            : default;
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
        return AsScript<T>(CEntityScriptLibrary.FindScript(this, Entity, ScriptClass));
    }

    private CEntityScript[] FindScriptsOfAbstract(FEntity Entity)
    {
        return CEntityScriptLibrary.FindScripts(this, Entity, ClassOf<CEntityScript>());
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
            ? CEntityScriptLibrary.FindScripts(this, Entity, ScriptClass)
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
        return AsScript<T>(CEntityScriptLibrary.AddScript(this, Entity, ClassOf<T>()));
    }

    public CEntityScript? AddScript(FEntity Entity, TSubclassOf<CEntityScript> ScriptClass)
    {
        return CEntityScriptLibrary.AddScript(this, Entity, ScriptClass) as CEntityScript;
    }

    public CEntityScript? AddScript(FEntity Entity, string ClassName)
    {
        return AddScript(Entity, TSubclassOf<CEntityScript>.FromName(ClassName));
    }

    public bool RemoveScript<T>(FEntity Entity) where T : class
    {
        CEntityScript? Script = GetScript<T>(Entity) as CEntityScript;
        return Script is not null && CEntityScriptLibrary.RemoveScript(this, Entity, Script);
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
}

// Which registry signal a subscription listens to, in the order the native connect export numbers them.
internal enum ERegistrySignal
{
    Construct,
    Destroy,
    Update,
}
