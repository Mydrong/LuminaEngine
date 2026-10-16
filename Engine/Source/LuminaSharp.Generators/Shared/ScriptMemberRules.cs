using System;

namespace LuminaSharp.ScriptProperties;

// The shape of a member as the rules need it, whatever is reading it, Roslyn symbols or System.Reflection.
internal enum EScriptMemberKind
{
    Field,

    // A C# property with a setter, or a get-only one exposing a native-owned view.
    WritableProperty,

    // A get-only property that is not a view, which has nothing native can store.
    ReadOnlyProperty,
}

// What one member becomes natively. Flag values are LuminaSharp.EPropertyFlags bits, kept numeric so this file needs nothing beyond the BCL.
internal readonly struct FScriptMemberRule
{
    public readonly bool bStored;
    public readonly bool bVisible;
    public readonly uint Flags;

    public FScriptMemberRule(bool bInStored, bool bInVisible, uint InFlags)
    {
        bStored = bInStored;
        bVisible = bInVisible;
        Flags = InFlags;
    }
}

// The one copy of what a [Property] makes of a member, read by the type library, the rewriter, the struct layout and the analyzer.
internal static class ScriptMemberRules
{
    public const string PropertyAttribute = "PropertyAttribute";

    public const string Editable = "Editable";
    public const string ReadOnly = "ReadOnly";
    public const string Hidden = "Hidden";
    public const string NoSerialize = "NoSerialize";
    public const string SaveGame = "SaveGame";
    public const string EditorOnly = "EditorOnly";
    public const string DuplicateTransient = "DuplicateTransient";
    public const string Sync = "Sync";
    public const string Bind = "Bind";

    private const uint ReadOnlyFlag = 1u << 1;
    private const uint NoSerializeFlag = 1u << 2;
    private const uint EditorOnlyFlag = 1u << 11;
    private const uint ReplicatedFlag = 1u << 12;
    private const uint DuplicateTransientFlag = 1u << 16;
    private const uint SaveGameFlag = 1u << 19;

    // HasSpecifier is null for a member with no [Property], and otherwise answers for a specifier or a named argument such as "Sync".
    public static FScriptMemberRule Classify(Func<string, bool>? HasSpecifier, EScriptMemberKind Kind, bool bOwnerIsNativeObject)
    {
        if (HasSpecifier == null || Kind == EScriptMemberKind.ReadOnlyProperty)
        {
            return default;
        }

        // Only an object has a native block to put a UI binding in, so a plain view model keeps its own.
        if (HasSpecifier(Bind) && !bOwnerIsNativeObject)
        {
            return default;
        }

        bool bVisible = !HasSpecifier(Hidden);
        uint Flags = (HasSpecifier(ReadOnly) ? ReadOnlyFlag : 0u)
                   | (HasSpecifier(NoSerialize) ? NoSerializeFlag : 0u)
                   | (HasSpecifier(EditorOnly) ? EditorOnlyFlag : 0u)
                   | (HasSpecifier(Sync) ? ReplicatedFlag : 0u)
                   | (HasSpecifier(DuplicateTransient) ? DuplicateTransientFlag : 0u)
                   | (HasSpecifier(SaveGame) ? SaveGameFlag : 0u);
        return new FScriptMemberRule(true, bVisible, Flags);
    }
}
