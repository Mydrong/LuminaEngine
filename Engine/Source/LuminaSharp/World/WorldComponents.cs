using System;
using System.Collections.Generic;
using LuminaSharp;

namespace Lumina;

public unsafe partial class CWorld
{
    public T? TryGetComponent<T>(FEntity Entity) where T : NativeStruct
    {
        uint* Epoch = null;
        IntPtr Pointer = Native.GetComponentTracked(WorldHandle, Entity.Id, ComponentOps<T>.Token, &Epoch);
        if (Pointer == IntPtr.Zero)
        {
            return null;
        }

        T? View = Wrapper<T>.Create(Pointer);
        View?.BindComponentView(WorldHandle, Entity.Id, ComponentOps<T>.Token, Epoch);
        return View;
    }

    // Another lookup of T on this thread rebinds the returned view to a different entity.
    public T? TryGetComponentUnsafe<T>(FEntity Entity) where T : NativeStruct
    {
        uint* Epoch = null;
        IntPtr Pointer = Native.GetComponentTracked(WorldHandle, Entity.Id, ComponentOps<T>.Token, &Epoch);
        T? View = ComponentOps<T>.ScratchView;
        if (Pointer == IntPtr.Zero)
        {
            View?.BindNativeHandle(IntPtr.Zero);
            return null;
        }

        if (View == null)
        {
            View = Wrapper<T>.Create(Pointer);
            ComponentOps<T>.ScratchView = View;
        }
        else
        {
            View.BindNativeHandle(Pointer);
        }

        View?.BindComponentView(WorldHandle, Entity.Id, ComponentOps<T>.Token, Epoch);
        return View;
    }

    // The pointer is invalid after pool relocation, component removal, or world destruction.
    public IntPtr TryGetComponentPointer<T>(FEntity Entity) where T : NativeStruct
        => Native.GetComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token);

    public T GetComponent<T>(FEntity Entity) where T : NativeStruct
    {
        return TryGetComponent<T>(Entity) ?? throw new InvalidOperationException($"Entity {Entity.Id} has no {typeof(T).Name}.");
    }

    public bool HasComponent<T>(FEntity Entity) where T : NativeStruct
    {
        return Native.HasComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token) != 0;
    }

    public T? GetOrEmplaceComponent<T>(FEntity Entity) where T : NativeStruct
    {
        IntPtr Pointer = Native.EmplaceComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token);
        if (Pointer == IntPtr.Zero)
        {
            return null;
        }
        return TryGetComponent<T>(Entity);
    }

    public bool RemoveComponent<T>(FEntity Entity) where T : NativeStruct
    {
        return Native.RemoveComponent(WorldHandle, Entity.Id, ComponentOps<T>.Token) != 0;
    }

    public RegistrySubscription OnConstruct<T>(Action<FEntity> Callback) where T : NativeStruct
        => Subscribe(ComponentOps<T>.Token, 0, Callback);
    public RegistrySubscription OnDestroy<T>(Action<FEntity> Callback) where T : NativeStruct
        => Subscribe(ComponentOps<T>.Token, 1, Callback);
    public RegistrySubscription OnUpdate<T>(Action<FEntity> Callback) where T : NativeStruct
        => Subscribe(ComponentOps<T>.Token, 2, Callback);
    public void PatchComponent<T>(FEntity Entity) where T : NativeStruct
        => Native.RegistryPatch(WorldHandle, Entity.Id, ComponentOps<T>.Token);

    private RegistrySubscription Subscribe(IntPtr Token, int Kind, Action<FEntity> Callback)
    {
        if (Token == IntPtr.Zero)
        {
            return RegistrySubscription.Empty;
        }

        IntPtr Listener = Native.RegistryConnect(WorldHandle, Token, Kind);
        if (Listener == IntPtr.Zero)
        {
            return RegistrySubscription.Empty;
        }
        IntPtr Signal = Native.RegistryGetSignalDelegate(Listener);
        DelegateBinding Binding;
        unsafe
        {
            Binding = DelegateBindings.Bind((void*)Signal,
                new PayloadInvoker<uint> { Handler = Id => Callback(new FEntity(Id)) });
        }

        if (!Binding.IsValid)
        {
            Native.RegistryDisconnect(WorldHandle, Token, Kind, Listener);
            return RegistrySubscription.Empty;
        }

        return new RegistrySubscription(WorldHandle, Token, Kind, Listener, Binding);
    }

    private static TSubclassOf<CEntityScript> ClassOf<T>() where T : CEntityScript
        => NativeClass<T>.Of.AsBase<CEntityScript>();
    public T? GetScript<T>(FEntity Entity) where T : CEntityScript
    {
        if (!ClassOf<T>().IsValid)
        {
            foreach (CEntityScript Script in FindScriptsOfAbstract(Entity))
            {
                if (Script is T Typed)
                {
                    return Typed;
                }
            }
            return null;
        }
        return AsScript<T>(CEntityScriptLibrary.FindScript(this, Entity, ClassOf<T>()));
    }

    private CEntityScript[] FindScriptsOfAbstract(FEntity Entity)
    {
        return CEntityScriptLibrary.FindScripts(this, Entity, ClassOf<CEntityScript>());
    }

    private static T? AsScript<T>(CEntityScript? Script) where T : CEntityScript
    {
        if (Script is T Typed)
        {
            return Typed;
        }
        return Script is null ? null : Wrapper<T>.ForObject(Script.Handle);
    }

    public List<T> GetScripts<T>(FEntity Entity) where T : CEntityScript
    {
        CEntityScript[] Scripts = ClassOf<T>().IsValid
            ? CEntityScriptLibrary.FindScripts(this, Entity, ClassOf<T>())
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

    public bool RemoveScript<T>(FEntity Entity) where T : CEntityScript
    {
        CEntityScript? Script = GetScript<T>(Entity);
        return Script is not null && CEntityScriptLibrary.RemoveScript(this, Entity, Script);
    }

    public bool HasScript<T>(FEntity Entity) where T : CEntityScript
    {
        return GetScript<T>(Entity) != null;
    }

    public View<T1> View<T1>(Exclude Filter = default)
        where T1 : NativeStruct
    {
        return new View<T1>(WorldHandle, ComponentOps<T1>.Token, Filter);
    }

    public View<T1> All<T1>(Exclude Filter = default) where T1 : NativeStruct => View<T1>(Filter);

    public View<T1, T2> View<T1, T2>(Exclude Filter = default)
        where T1 : NativeStruct
        where T2 : NativeStruct
    {
        return new View<T1, T2>(WorldHandle, ComponentOps<T1>.Token, ComponentOps<T2>.Token, Filter);
    }

    public View<T1, T2, T3> View<T1, T2, T3>(Exclude Filter = default)
        where T1 : NativeStruct
        where T2 : NativeStruct
        where T3 : NativeStruct
    {
        return new View<T1, T2, T3>(WorldHandle, ComponentOps<T1>.Token, ComponentOps<T2>.Token, ComponentOps<T3>.Token, Filter);
    }

    public View<T1, T2, T3, T4> View<T1, T2, T3, T4>(Exclude Filter = default)
        where T1 : NativeStruct
        where T2 : NativeStruct
        where T3 : NativeStruct
        where T4 : NativeStruct
    {
        return new View<T1, T2, T3, T4>(WorldHandle, ComponentOps<T1>.Token, ComponentOps<T2>.Token, ComponentOps<T3>.Token, ComponentOps<T4>.Token, Filter);
    }
}
