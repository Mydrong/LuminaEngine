using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;

namespace LuminaSharp;

// Compares every [NativeLayout] mirror's managed size with the native one at boot.
internal static unsafe partial class LayoutValidator
{
    private static readonly MethodInfo SizeOf =
        typeof(Unsafe).GetMethod(nameof(Unsafe.SizeOf), BindingFlags.Public | BindingFlags.Static)!;

    // Namespaces where every value struct is a native mirror and must carry [NativeLayout].
    private static readonly string[] StrictNamespaces = { "LuminaSharp.Rendering" };

    /// <summary>Validate every mirror. Returns true if all match; otherwise logs each offender and returns false.</summary>
    internal static bool ValidateAll()
    {
        Type[] Types;
        try
        {
            Types = typeof(LayoutValidator).Assembly.GetTypes();
        }
        catch (ReflectionTypeLoadException Ex)
        {
            Types = Array.FindAll(Ex.Types, T => T != null)!;
        }

        int Validated = 0;
        StringBuilder Failures = new();

        foreach (Type Type in Types)
        {
            NativeLayoutAttribute? Attribute = Type.GetCustomAttribute<NativeLayoutAttribute>();

            if (Attribute == null)
            {
                if (Type.IsValueType && !Type.IsEnum && !Type.IsPrimitive && !Type.IsGenericType
                    && !Type.IsNested && Type.Name.IndexOf('<') < 0
                    && Type.Namespace is string Ns && Array.IndexOf(StrictNamespaces, Ns) >= 0)
                {
                    Failures.Append($"\n  - {Type.FullName}: a struct in '{Ns}' is missing [NativeLayout].");
                }
                continue;
            }

            if (!Type.IsValueType)
            {
                Failures.Append($"\n  - {Type.FullName}: [NativeLayout] is only valid on value types.");
                continue;
            }

            int ManagedSize = (int)SizeOf.MakeGenericMethod(Type).Invoke(null, null)!;
            int NativeBytes = Lumina.CScriptInteropLibrary.GetMirrorSize(Attribute.NativeType);

            if (NativeBytes < 0)
            {
                Failures.Append($"\n  - {Type.FullName}: native type '{Attribute.NativeType}' is not registered (LE_REGISTER_LAYOUT missing).");
            }
            else if (NativeBytes != ManagedSize)
            {
                Failures.Append($"\n  - {Type.FullName}: SIZE MISMATCH, C# is {ManagedSize} bytes, native '{Attribute.NativeType}' is {NativeBytes} bytes.");
            }
            else
            {
                Validated++;
            }
        }

        // EPropertyType mirrors native kinds sent as bytes on the schema wire, so each value and the member count are checked.
        Array PropertyTypes = Enum.GetValues(typeof(EPropertyType));
        foreach (EPropertyType Value in PropertyTypes)
        {
            int NativeValue = Lumina.CScriptInteropLibrary.GetPropertyTypeValue(Value.ToString());
            if (NativeValue != (int)Value)
            {
                Failures.Append($"\n  - EPropertyType.{Value} is {(int)Value} in C# but native reports {NativeValue}.");
            }
        }
        int NativeTypeCount = Lumina.CScriptInteropLibrary.GetPropertyTypeCount();
        if (NativeTypeCount != PropertyTypes.Length)
        {
            Failures.Append($"\n  - EPropertyType has {PropertyTypes.Length} members in C# but native EPropertyTypeFlags has {NativeTypeCount}.");
        }

        // A wrong EPropertyFlags bit silently stops a field replicating or saving, so each is checked at startup.
        int FlagsChecked = 0;
        foreach (EPropertyFlags Value in Enum.GetValues(typeof(EPropertyFlags)))
        {
            if (Value == EPropertyFlags.None)
            {
                continue;
            }

            int NativeValue = Lumina.CScriptInteropLibrary.GetPropertyFlagValue(Value.ToString());
            if (NativeValue != (int)Value)
            {
                Failures.Append($"\n  - EPropertyFlags.{Value} is {(int)Value} in C# but native reports {NativeValue}.");
            }
            ++FlagsChecked;
        }

        // The parameter half rides the same wire as the author-facing half, so it needs the same guard.
        foreach (EScriptParamFlags Value in Enum.GetValues(typeof(EScriptParamFlags)))
        {
            if (Value == EScriptParamFlags.None)
            {
                continue;
            }

            int NativeValue = Lumina.CScriptInteropLibrary.GetPropertyFlagValue(Value.ToString());
            if (NativeValue != (int)Value)
            {
                Failures.Append($"\n  - EScriptParamFlags.{Value} is {(int)Value} in C# but native reports {NativeValue}.");
            }
            ++FlagsChecked;
        }

        ValidateModules(typeof(LayoutValidator).Assembly, Failures);

        if (Failures.Length > 0)
        {
            Debug.LogError($"FATAL: C#/C++ interop layout validation FAILED, refusing to start C# (a mismatched blittable layout or enum would corrupt memory). Fix the C# mirror or the native type:{Failures}");
            return false;
        }

        Debug.Log($"Interop layout validated: {Validated} native mirrors + {PropertyTypes.Length} EPropertyType + {FlagsChecked} EPropertyFlags members match native.");
        return true;
    }

    // A module that is not loaded has nothing to disagree with, so only a loaded module with another hash fails.
    internal static void ValidateModules(Assembly Assembly, StringBuilder Failures)
    {
        foreach (NativeModuleLayoutAttribute Stamp in Assembly.GetCustomAttributes<NativeModuleLayoutAttribute>())
        {
            ulong NativeHash = Lumina.CScriptInteropLibrary.GetModuleLayoutHash(Stamp.Module);
            if (NativeHash != 0 && NativeHash != Stamp.Hash)
            {
                Failures.Append($"\n  - {Assembly.GetName().Name}: the '{Stamp.Module}' bindings were generated for another build of that module (0x{Stamp.Hash:X16} vs native 0x{NativeHash:X16}). Rebuild so the reflector regenerates them.");
            }
        }
    }
}
