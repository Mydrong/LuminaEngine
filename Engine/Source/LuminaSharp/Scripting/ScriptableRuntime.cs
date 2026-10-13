using System;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;

namespace LuminaSharp;

/// <summary>
/// Hosts live managed instances of C# subclasses of <c>REFLECT(Scriptable)</c> native CObjects. Generic over
/// any Scriptable type, with no per-type hosting code: native creates the CObject (a minted CClass), this
/// runtime instantiates the matching C# subclass and pairs it to the native object via a GCHandle.
///
/// It keeps no handle list of its own. Each handle lives in its native object's managed-instance slot
/// (Core/Object/ManagedInstance.h), which frees it when the object dies and drains the whole table before the
/// collectible ALC unloads -- so there is exactly one owner and no second bookkeeping to keep in step.
/// Per-instance dispatch is driven directly by the Reflector-generated native shim, not from here.
/// </summary>
internal sealed class ScriptableRuntime
{
    private const byte AbstractFlag = 1;

    private readonly TypeLibrary Library;

    // EntityScripts are created here but resolved and destroyed through EntityScriptRuntime's handle set.
    private readonly EntityScriptRuntime EntityScripts;

    // Cached per user type: the names of the ScriptEvents that type overrides, joined once.
    private readonly Dictionary<Type, string> OverriddenEventsByType = new();

    public ScriptableRuntime(TypeLibrary Library, EntityScriptRuntime EntityScripts)
    {
        this.Library = Library;
        this.EntityScripts = EntityScripts;
    }

    public IReadOnlyCollection<string> TypeNames => Library.ScriptableTypeNames;

    /// <summary>Reports each discovered Scriptable C# type as (full name, native base class name, the names
    /// of the events it overrides) to a native sink, so the host can mint a CClass deriving from that native
    /// base. The native base name is the <c>[NativeType]</c> name of the nearest <c>[ScriptableType]</c>
    /// wrapper in the base chain. Overrides are named rather than numbered: the host turns each into a real
    /// function on the minted class, so there is no index to agree on and no ceiling to hit.</summary>
    public unsafe void Enumerate(IntPtr Sink, IntPtr Context)
    {
        if (Sink == IntPtr.Zero)
        {
            return;
        }

        var Add = (delegate* unmanaged[Stdcall]<IntPtr, byte*, int, byte*, int, byte*, int, byte, byte*, int, void>)Sink;
        Span<byte> NameScratch = stackalloc byte[256];
        Span<byte> BaseScratch = stackalloc byte[256];
        Span<byte> OverrideScratch = stackalloc byte[1024];
        Span<byte> MetaScratch = stackalloc byte[512];
        // Parents first, since a class is minted under its C# parent and stacks its block after that parent's.
        foreach (Type Type in Library.ScriptableTypes.OrderBy(Depth))
        {
            string FullName = NativeTypeName.Of(Type);
            Type? MintedParent = Library.MintedParentOf(Type);
            string? NativeBase = MintedParent != null ? NativeTypeName.Of(MintedParent) : ScriptableNativeBaseName(Type);
            if (NativeBase == null)
            {
                continue;
            }

            Interop.FInteropString Name = new(FullName, NameScratch);
            Interop.FInteropString Base = new(NativeBase, BaseScratch);
            Interop.FInteropString Overrides = new(GetOverriddenEvents(Type), OverrideScratch);
            Interop.FInteropString Meta = new(GetClassMeta(Type), MetaScratch);
            try
            {
                Add(Context, Name.Pointer, Name.Length, Base.Pointer, Base.Length,
                    Overrides.Pointer, Overrides.Length, (byte)(Type.IsAbstract ? AbstractFlag : 0),
                    Meta.Pointer, Meta.Length);
            }
            finally
            {
                Meta.Free();
                Overrides.Free();
                Name.Free();
                Base.Free();
            }
        }
    }

    /// <summary>Reports each (prior name, current name) pair from the <c>[Alias]</c> attributes on script
    /// classes, so the host can record where a renamed class went. Separate from <see cref="Enumerate"/>
    /// rather than folded into it because that sink's arity is part of the native ABI.</summary>
    public unsafe void EnumerateAliases(IntPtr Sink, IntPtr Context)
    {
        if (Sink == IntPtr.Zero)
        {
            return;
        }

        var Add = (delegate* unmanaged[Stdcall]<IntPtr, byte*, int, byte*, int, void>)Sink;
        Span<byte> OldScratch = stackalloc byte[256];
        Span<byte> NewScratch = stackalloc byte[256];
        foreach (KeyValuePair<string, string> Pair in Library.ClassAliases)
        {
            Interop.FInteropString Old = new(Pair.Key, OldScratch);
            Interop.FInteropString New = new(Pair.Value, NewScratch);
            try
            {
                Add(Context, Old.Pointer, Old.Length, New.Pointer, New.Length);
            }
            finally
            {
                Old.Free();
                New.Free();
            }
        }
    }

    /// <summary>Instantiates the named Scriptable subclass, pairs it to the already-created native object, and
    /// returns a STRONG GCHandle (IntPtr.Zero on failure).
    ///
    /// The caller stores the handle in the native object's managed-instance slot
    /// (Core/Object/ManagedInstance.h), which is its only owner: the object's destructor frees it, and the
    /// script teardown contract drains the whole table before the collectible ALC unloads. Strong rather than
    /// weak because this instance holds user state for as long as the native object lives -- letting the GC
    /// take it between dispatches would silently reset the script's fields.</summary>
    public IntPtr Create(string TypeName, ulong NativePtr)
    {
        Type? Type = Library.GetScriptable(TypeName);
        if (Type == null || Type.IsAbstract)
        {
            Native.Log(ELogLevel.Warn, $"Scriptable type not found or abstract: '{TypeName}'.");
            return IntPtr.Zero;
        }

        if (Activator.CreateInstance(Type) is not NativeObject Instance)
        {
            Native.Log(ELogLevel.Error, $"Failed to create Scriptable '{TypeName}'.");
            return IntPtr.Zero;
        }

        Instance.BindNativeHandle(new IntPtr(unchecked((long)NativePtr)));

        GCHandle Allocated = GCHandle.Alloc(Instance);

        // The one point every script instance passes through, and it runs before the dispatch that created it.
        if (Instance is Lumina.CEntityScript Script)
        {
            PrepareEntityScript(Script, TypeName, Type);
            EntityScripts.Adopt(Allocated);
        }
        return GCHandle.ToIntPtr(Allocated);
    }

    // Description must be set before any dispatch: OnAction and the profiler labels both read it.
    private void PrepareEntityScript(Lumina.CEntityScript Script, string TypeName, Type Type)
    {
        Script.Description = Library.Describe(Type);

        // Defensive: the driver always SetOwner's before the first dispatch that creates this instance.
        if (Script.Entity.IsNull)
        {
            return;
        }

        try
        {
            Script.Description.EnsureInputComponent(Script);
        }
        catch (Exception Exception)
        {
            Native.Log(ELogLevel.Error, $"CEntityScript component injection threw: {Exception}");
        }
    }

    /// <summary>
    /// Runs a type's declared [Property] initializers into its class default object, once, at mint time.
    ///
    /// The CDO deliberately never keeps a managed instance -- it is never dispatched to -- so one is created
    /// here, bound just long enough for the generated <c>__ApplyScriptDefaults</c> to write through its
    /// accessors into the CDO's native block, and then dropped. Every later instance is copied from that
    /// block by CClass::ConstructScriptProperties, which is how a script gets defaults at all: a minted class
    /// has no C++ constructor to carry them.
    /// </summary>
    public void ApplyDefaults(string TypeName, ulong NativeDefaultObject)
    {
        // A script data table row has no class default object, so its initializers land in the minted struct's defaults block.
        if (Library.GetDataStruct(TypeName) is { } Row)
        {
            if (Activator.CreateInstance(Row.Type) is NativeStruct RowInstance)
            {
                RowInstance.BindNativeHandle(new IntPtr(unchecked((long)NativeDefaultObject)));
                try
                {
                    RowInstance.ApplyScriptDefaults();
                }
                catch (Exception Exception)
                {
                    Native.Log(ELogLevel.Error, $"Script defaults for '{TypeName}' threw: {Exception.Message}");
                }
            }
            return;
        }

        // An abstract class is never instantiated, and its subclasses' generated defaults chain through its own.
        Type? Type = Library.GetScriptable(TypeName);
        if (Type == null || Type.IsAbstract || Activator.CreateInstance(Type) is not NativeObject Instance)
        {
            return;
        }

        Instance.BindNativeHandle(new IntPtr(unchecked((long)NativeDefaultObject)));
        try
        {
            Instance.ApplyScriptDefaults();
        }
        catch (Exception Exception)
        {
            Native.Log(ELogLevel.Error, $"Script defaults for '{TypeName}' threw: {Exception.Message}");
        }
    }

    // The REFLECT specifiers for the minted class as lines of key, tab, value, nearest declaration first.
    private string GetClassMeta(Type Type)
    {
        var Lines = new System.Text.StringBuilder();
        var Written = new HashSet<string>();
        foreach (Type Declaring in Library.ChainUpToMintedParent(Type))
        {
            if (Declaring.GetCustomAttribute<ReflectAttribute>(inherit: false) is { } Reflect)
            {
                foreach (KeyValuePair<string, string> Specifier in Reflect.Specifiers())
                {
                    AppendMeta(Lines, Written, Specifier.Key, Specifier.Value);
                }
            }
            foreach (ClassMetaAttribute Meta in Declaring.GetCustomAttributes<ClassMetaAttribute>(inherit: false))
            {
                AppendMeta(Lines, Written, Meta.Key, Meta.Value);
            }
        }
        return Lines.ToString();
    }

    private static void AppendMeta(System.Text.StringBuilder Lines, HashSet<string> Written, string Key, string Value)
    {
        if (Written.Add(Key))
        {
            Lines.Append(Key).Append('\t').Append(Value.Replace('\n', ' ').Replace('\t', ' ')).Append('\n');
        }
    }

    // An override moves the method's DeclaringType out of LuminaSharp.dll, which is what identifies one.
    private string GetOverriddenEvents(Type Type)
    {
        if (OverriddenEventsByType.TryGetValue(Type, out string? Cached))
        {
            return Cached;
        }

        var Names = new List<string>();
        Assembly Engine = typeof(ScriptableRuntime).Assembly;
        foreach (MethodInfo Method in Type.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic))
        {
            if (Method.GetCustomAttribute<ScriptEventAttribute>(inherit: true) != null
                && Method.DeclaringType is { } Decl && !Names.Contains(Method.Name)
                && (Decl.Assembly != Engine || EngineOverrideNeedsDispatch(Method, Type)))
            {
                Names.Add(Method.Name);
            }
        }

        string Joined = string.Join(';', Names);
        OverriddenEventsByType[Type] = Joined;
        return Joined;
    }

    // An event the script leaves alone still dispatches when its declaring class names it in a [DispatchWhen] that accepts the script.
    private bool EngineOverrideNeedsDispatch(MethodInfo Method, Type ScriptType)
    {
        if (Method.DeclaringType is not { } Declaring
            || Declaring.GetCustomAttributes<DispatchWhenAttribute>(inherit: false).FirstOrDefault(When => When.Event == Method.Name) is not { } When)
        {
            return false;
        }
        MethodInfo? Predicate = Declaring.GetMethod(When.Predicate, BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic,
            new[] { typeof(TypeLibrary), typeof(Type) });
        return Predicate != null && Predicate.Invoke(null, new object[] { Library, ScriptType }) is true;
    }

    private static int Depth(Type Type)
    {
        int Count = 0;
        for (Type? Base = Type.BaseType; Base != null; Base = Base.BaseType)
        {
            ++Count;
        }
        return Count;
    }

    // The [NativeType] name of the nearest [ScriptableType]-marked wrapper above Type, or null if none.
    private static string? ScriptableNativeBaseName(Type Type)
    {
        for (Type? Base = Type.BaseType; Base != null; Base = Base.BaseType)
        {
            if (Base.GetCustomAttribute<ScriptableTypeAttribute>(inherit: false) != null)
            {
                NativeTypeAttribute? Native = Base.GetCustomAttribute<NativeTypeAttribute>(inherit: false);
                return Native?.Name ?? Base.Name;
            }
        }
        return null;
    }
}
