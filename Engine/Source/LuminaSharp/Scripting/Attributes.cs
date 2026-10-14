using System;
using Lumina;
using System.Collections.Generic;
using System.Reflection;

namespace LuminaSharp;

// The C# spelling of a C++ REFLECT(...), and what registers a class extending a native struct, as REFLECT does for a C++ struct.
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum, AllowMultiple = false, Inherited = false)]
public sealed class ReflectAttribute : Attribute
{
    private EScriptUpdatePhase UpdatePhaseValue;
    private bool InteractiveValue;
    private bool ShowOnStartValue = true;

    public string? DisplayName { get; set; }

    public string? Category { get; set; }

    public string? ToolTip { get; set; }

    // The name an .rml's data-model attribute uses for this class. The class name when no class in the chain sets one.
    public string? DataModel { get; set; }

    // The .rml a UI script shows when the entity leaves its Document property empty.
    public string? UIDocument { get; set; }

    // Backs a settings class with this config file, which the config system loads and saves its properties to.
    public string? ConfigFile { get; set; }

    // Puts every asset of this class and its subclasses in the named asset group, the same as a C++ REFLECT(Group = ...).
    public string? Group { get; set; }

    public bool HideInComponentList { get; set; }

    public bool HideInDetails { get; set; }

    // Keeps the class out of the node graph's placeable node registry.
    public bool NotPlaceable { get; set; }

    // OnUpdate and OnFixedUpdate touch only this entity and change no structure, so the driver runs it in parallel.
    public bool ParallelUpdate { get; set; }

    // Runs only on the host, a dedicated server or a standalone game. Every realm set must hold.
    public bool HostOnly { get; set; }

    // Runs only on a client joined to a host, never on the host or in a standalone game.
    public bool ClientOnly { get; set; }

    // Runs only where something is drawn or played, never on a dedicated server, a bot or a headless process.
    public bool Cosmetic { get; set; }

    public EScriptUpdatePhase UpdatePhase
    {
        get => UpdatePhaseValue;
        set
        {
            UpdatePhaseValue = value;
            HasUpdatePhase = true;
        }
    }

    // A UI script's starting Interactive value, which a C++ subclass would set in its constructor.
    public bool Interactive
    {
        get => InteractiveValue;
        set
        {
            InteractiveValue = value;
            HasInteractive = true;
        }
    }

    // A UI script's starting ShowOnStart value, which a C++ subclass would set in its constructor.
    public bool ShowOnStart
    {
        get => ShowOnStartValue;
        set
        {
            ShowOnStartValue = value;
            HasShowOnStart = true;
        }
    }

    internal bool HasUpdatePhase { get; private set; }

    internal bool HasInteractive { get; private set; }

    internal bool HasShowOnStart { get; private set; }

    // The nearest [Reflect] up Type's chain that sets what Picks reads, mirroring how native reads class metadata.
    internal static ReflectAttribute? Nearest(Type? Type, Func<ReflectAttribute, bool> Picks)
    {
        for (Type? Current = Type; Current != null; Current = Current.BaseType)
        {
            if (Current.GetCustomAttribute<ReflectAttribute>(inherit: false) is { } Reflect && Picks(Reflect))
            {
                return Reflect;
            }
        }
        return null;
    }

    internal static string DataModelOf(Type Type) => Nearest(Type, Reflect => Reflect.DataModel != null)?.DataModel ?? Type.Name;

    // Specifier name and value as C++ REFLECT spells them, with an empty value for a flag.
    internal IEnumerable<KeyValuePair<string, string>> Specifiers()
    {
        if (DisplayName != null)
        {
            yield return new(nameof(DisplayName), DisplayName);
        }
        if (Category != null)
        {
            yield return new(nameof(Category), Category);
        }
        if (ToolTip != null)
        {
            yield return new(nameof(ToolTip), ToolTip);
        }
        if (DataModel != null)
        {
            yield return new(nameof(DataModel), DataModel);
        }
        if (UIDocument != null)
        {
            yield return new(nameof(UIDocument), UIDocument);
        }
        if (ConfigFile != null)
        {
            yield return new(nameof(ConfigFile), ConfigFile);
        }
        if (Group != null)
        {
            yield return new(nameof(Group), Group);
        }
        if (HideInComponentList)
        {
            yield return new(nameof(HideInComponentList), "");
        }
        if (HideInDetails)
        {
            yield return new(nameof(HideInDetails), "");
        }
        if (NotPlaceable)
        {
            yield return new(nameof(NotPlaceable), "");
        }
        if (ParallelUpdate)
        {
            yield return new(nameof(ParallelUpdate), "");
        }
        if (HostOnly)
        {
            yield return new(nameof(HostOnly), "");
        }
        if (ClientOnly)
        {
            yield return new(nameof(ClientOnly), "");
        }
        if (Cosmetic)
        {
            yield return new(nameof(Cosmetic), "");
        }
        if (HasUpdatePhase)
        {
            yield return new(nameof(UpdatePhase), UpdatePhase.ToString());
        }
    }
}

// Class metadata an engine C# base hands to every script class minted beneath it, for a key no REFLECT specifier spells.
[AttributeUsage(AttributeTargets.Class, AllowMultiple = true, Inherited = false)]
internal sealed class ClassMetaAttribute : Attribute
{
    public ClassMetaAttribute(string Key, string Value = "")
    {
        this.Key = Key;
        this.Value = Value;
    }

    public string Key { get; }

    public string Value { get; }
}

// Dispatches Event to a script type that does not override it when the static bool(TypeLibrary, Type) Predicate on this class accepts the type, for engine work in a pre-event hook.
[AttributeUsage(AttributeTargets.Class, AllowMultiple = true, Inherited = false)]
internal sealed class DispatchWhenAttribute : Attribute
{
    public DispatchWhenAttribute(string Event, string Predicate)
    {
        this.Event = Event;
        this.Predicate = Predicate;
    }

    public string Event { get; }

    public string Predicate { get; }
}

/// <summary>Declares a prior name this element was serialized under, so saved data survives a rename. Repeatable.</summary>
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = true, Inherited = false)]
public sealed class AliasAttribute : Attribute
{
    public AliasAttribute(string Name)
    {
        this.Name = Name;
    }

    /// <summary>A prior name, the type's name on a class, struct or enum, or a member name on a property.</summary>
    public string Name { get; }
}

/// <summary>Resets this [Property] (or every [Property] on the class) to its default on a C# hot reload instead of carrying the previous value into the reloaded instance.</summary>
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
public sealed class SkipHotReloadAttribute : Attribute
{
}

/// <summary>
/// Reflected property flags. A hand-written mirror of native <c>Lumina::EPropertyFlags</c>, single-sourced
/// there from <c>EPropertyFlags.inl</c> and checked member-by-member at bootstrap: a wrong bit is a field that
/// quietly stops replicating rather than a crash, so it fails loudly at startup instead.
/// </summary>
[Flags]
public enum EPropertyFlags : uint
{
    None               = 0,

    /// <summary>Shown in the details panel. Implied by <see cref="PropertyAttribute"/>.</summary>
    Editable           = 1u << 0,
    /// <summary>Shown but not editable.</summary>
    ReadOnly           = 1u << 1,
    /// <summary>Never written to a package.</summary>
    NoSerialize        = 1u << 2,
    /// <summary>Not writable through reflection at all.</summary>
    Const              = 1u << 3,
    /// <summary>Stripped from cooked packages.</summary>
    EditorOnly         = 1u << 11,
    /// <summary>Participates in network replication.</summary>
    Replicated         = 1u << 12,
    /// <summary>Duplication resets this to its default rather than copying it.</summary>
    DuplicateTransient = 1u << 16,
    /// <summary>Written into save games. The same as <see cref="SaveGameAttribute"/>.</summary>
    SaveGame           = 1u << 19,
}

// The parameter half of native EPropertyFlags, kept apart because neither is anything a [Property] declares.
[Flags]
internal enum EScriptParamFlags : uint
{
    None     = 0,

    // The caller reads this frame slot back after the call, which is what the dispatcher writes back into.
    OutParam = 1u << 17,

    // An out parameter the callee also sees the incoming value of, so C# ref rather than C# out.
    RefParam = 1u << 18,
}

/// <summary>
/// Exposes a script field/property to the editor (and saves it).
/// </summary>
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
public sealed class PropertyAttribute : Attribute
{
    /// <summary>Inspector group.</summary>
    public string? Category { get; set; }

    /// <summary>Hover help.</summary>
    public string? Tooltip { get; set; }

    /// <summary>Display label override (defaults to the member name).</summary>
    public string? Name { get; set; }

    /// <summary>Slider/drag lower bound (NaN = unbounded).</summary>
    public float Min { get; set; } = float.NaN;

    /// <summary>Slider/drag upper bound (NaN = unbounded).</summary>
    public float Max { get; set; } = float.NaN;

    /// <summary>Unit suffix shown after the value (e.g. "m/s").</summary>
    public string? Units { get; set; }

    /// <summary>Draw a color picker for a Vector3/Vector4 value instead of drag fields.</summary>
    public bool Color { get; set; }

    // Turns a string into a picker of those file kinds, such as "rml" or "rml,rcss", the same as a C++ PROPERTY(AssetType = ...).
    public string? AssetType { get; set; }

    // Soft references in this field load with their owner when the named bundle is asked for, the same as a C++ PROPERTY(Bundle = ...).
    public string? Bundle { get; set; }

    // Narrows an SAssetId field's picker to one asset group, the same as a C++ PROPERTY(Group = ...).
    public string? Group { get; set; }

    /// <summary>
    /// Reflected flags for this property, on top of the Editable one <see cref="PropertyAttribute"/> implies.
    ///
    /// This is how a script field says the things the engine already understood but C# could not express:
    /// <c>Replicated</c>, <c>ReadOnly</c>, <c>EditorOnly</c>, <c>Const</c>, <c>DuplicateTransient</c>.
    /// </summary>
    public EPropertyFlags Flags { get; set; } = EPropertyFlags.None;

    public bool HasMin => !float.IsNaN(Min);
    public bool HasMax => !float.IsNaN(Max);
}

/// <summary>
/// Persists a field/property with the entity without exposing it in the inspector.
/// </summary>
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
public sealed class SerializeAttribute : Attribute
{
}

/// <summary>
/// Written into save games whole, and stored natively without [Property], so a save-only field stays out of the inspector.
/// </summary>
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
public sealed class SaveGameAttribute : Attribute
{
}

/// <summary>Marks a field/property as never serialized or shown, even if otherwise eligible.</summary>
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
public sealed class HideAttribute : Attribute
{
}


/// <summary>Marks a generated wrapper method as a native reflected virtual that a C# subclass of a
/// <c>REFLECT(Scriptable)</c> class may override. Emitted by the Reflector, never hand-written. <see cref="Index"/>
/// is the override-flag bit, aligned by construction with the native forwarding shim's per-method bit, so the
/// runtime can tell native which events the subclass actually overrides.</summary>
[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class ScriptEventAttribute : Attribute
{
    public ScriptEventAttribute(int Index)
    {
        this.Index = Index;
    }

    /// <summary>The override-flag bit index for this event within its declaring Scriptable class.</summary>
    public int Index { get; }
}

/// <summary>
/// Publishes this method as a reflected function on the minted class, so native and other scripts can find
/// it by name and call it without a generated binding.
/// </summary>
/// <remarks>
/// Parameters and the return value become real FPropertys describing the call frame, so they are limited to
/// kinds the reflection system can describe. A method whose signature cannot be described is reported at
/// mint time and left unreflected rather than half-bound.
/// </remarks>
[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class ScriptFunctionAttribute : Attribute
{
}

/// <summary>Exposes a parameterless method as a clickable button in the script component's inspector.
/// Clicking it invokes the method on the live script instance (only while the game is running). Methods
/// taking arguments are ignored with a warning.</summary>
[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class ButtonAttribute : Attribute
{
    public ButtonAttribute()
    {
    }

    public ButtonAttribute(string Label)
    {
        this.Label = Label;
    }

    /// <summary>Button text override (defaults to the method name).</summary>
    public string? Label { get; set; }

    /// <summary>Hover help.</summary>
    public string? Tooltip { get; set; }
}
