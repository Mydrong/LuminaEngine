using System;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;

namespace LuminaSharp;

// Pairs each native object of a minted class with an instance of its C# subclass, the handle living in the object's managed-instance slot.
internal sealed class ScriptableRuntime
{
    private const byte AbstractFlag = 1;

    private readonly TypeLibrary Library;
    private readonly IReadOnlyDictionary<string, string> SourcePaths;

    // EntityScripts are created here but resolved and destroyed through EntityScriptRuntime's handle set.
    internal EntityScriptRuntime EntityScripts { get; }

    // Cached per user type: the names of the ScriptEvents that type overrides, joined once.
    private readonly Dictionary<Type, List<string>> OverriddenEventsByType = new();

    public ScriptableRuntime(TypeLibrary Library, EntityScriptRuntime EntityScripts,
        IReadOnlyDictionary<string, string> SourcePaths)
    {
        this.Library = Library;
        this.EntityScripts = EntityScripts;
        this.SourcePaths = SourcePaths;
    }

    public IReadOnlyCollection<string> TypeNames => Library.ScriptableTypeNames;

    // The C# parent's name when that class is minted too, else the native class the script derives from.
    internal string? NativeBaseOf(Type Type)
    {
        Type? MintedParent = Library.MintedParentOf(Type);
        return MintedParent != null ? NativeTypeName.Of(MintedParent) : (TypeLibrary.NativeClassBaseOf(Type) is { } Native ? NativeTypeName.Of(Native) : null);
    }

    // Creates the C# instance for a native object as a strong handle the object's managed-instance slot owns, strong since it holds the script's state.
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
        Instance.OnScriptInstanceCreated(this, Allocated);
        return GCHandle.ToIntPtr(Allocated);
    }

    // Runs a type's C# initializers into the defaults native copies every instance from, the one way any minted type gets defaults.
    public void ApplyDefaults(string TypeName, ulong NativeDefaultObject)
    {
        nint Defaults = new IntPtr(unchecked((long)NativeDefaultObject));
        if (Library.GetScriptStruct(TypeName) is { } Row)
        {
            try
            {
                // A struct living in native memory writes through its own accessors, and a plain value is constructed and written whole.
                if (Row.Type.GetMethod("__ApplyScriptDefaults", BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public) is { } Apply)
                {
                    Apply.Invoke(null, new object[] { new NativeRef(Defaults) });
                }
                else
                {
                    ScriptStructLayout.WriteBoxed(Row.Type, Defaults, Activator.CreateInstance(Row.Type));
                }
            }
            catch (Exception Exception)
            {
                Native.Log(ELogLevel.Error, $"Script defaults for '{TypeName}' threw: {(Exception.InnerException ?? Exception).Message}");
            }
            return;
        }

        // An abstract class is never instantiated, and its subclasses' generated defaults chain through its own.
        Type? Type = Library.GetScriptable(TypeName);
        if (Type == null || Type.IsAbstract || Activator.CreateInstance(Type) is not NativeObject Instance)
        {
            return;
        }

        Instance.BindNativeHandle(Defaults);
        try
        {
            Instance.ApplyScriptDefaults();
        }
        catch (Exception Exception)
        {
            Native.Log(ELogLevel.Error, $"Script defaults for '{TypeName}' threw: {Exception.Message}");
        }
    }

    // The REFLECT specifiers for the minted class, nearest declaration first, so a subclass overrides what it inherits.
    internal List<KeyValuePair<string, string>> ClassMeta(Type Type)
    {
        var Meta = new List<KeyValuePair<string, string>>();
        var Written = new HashSet<string>();
        void Add(string Key, string Value)
        {
            if (Written.Add(Key))
            {
                Meta.Add(new KeyValuePair<string, string>(Key, Value));
            }
        }

        if (SourcePaths.TryGetValue(Type.FullName ?? Type.Name, out string? SourcePath))
        {
            Add("SourceFile", SourcePath);
        }
        foreach (Type Declaring in Library.ChainUpToMintedParent(Type))
        {
            if (Declaring.GetCustomAttribute<ReflectAttribute>(inherit: false) is { } Reflect)
            {
                foreach (KeyValuePair<string, string> Specifier in Reflect.Specifiers())
                {
                    Add(Specifier.Key, Specifier.Value);
                }
            }
            foreach (ClassMetaAttribute Declared in Declaring.GetCustomAttributes<ClassMetaAttribute>(inherit: false))
            {
                Add(Declared.Key, Declared.Value);
            }
        }
        return Meta;
    }

    // An override moves the method's DeclaringType out of LuminaSharp.dll, which is what identifies one.
    internal IReadOnlyList<string> OverriddenEvents(Type Type)
    {
        if (OverriddenEventsByType.TryGetValue(Type, out List<string>? Cached))
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

        OverriddenEventsByType[Type] = Names;
        return Names;
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
}
