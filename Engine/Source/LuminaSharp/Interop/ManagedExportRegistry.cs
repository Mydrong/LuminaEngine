using System;
using System.Collections.Generic;

namespace LuminaSharp;

// Native resolves managed functions here by name, engine exports living for the process and script exports cleared each reload.
public static class ManagedExportRegistry
{
    private static readonly Dictionary<string, IntPtr> EngineExports = new(StringComparer.Ordinal);
    private static readonly Dictionary<string, IntPtr> ScriptExports = new(StringComparer.Ordinal);

    /// <summary>Registers a permanent engine export (LuminaSharp.dll). Called by generated bootstrap code.</summary>
    public static void RegisterEngine(string Name, IntPtr Pointer)
    {
        EngineExports[Name] = Pointer;
    }

    // Registers a script export from a [ModuleInitializer], dropped on the next reload since its pointer would dangle.
    public static void Register(string Name, IntPtr Pointer)
    {
        ScriptExports[Name] = Pointer;
    }

    // Drops every script export as a generation unloads, leaving engine exports untouched.
    public static void ClearScriptExports()
    {
        ScriptExports.Clear();
    }

    // An export by name, or zero, an engine export winning so a plugin cannot shadow a core entry.
    public static IntPtr Resolve(string Name)
    {
        if (EngineExports.TryGetValue(Name, out IntPtr Engine))
        {
            return Engine;
        }
        return ScriptExports.TryGetValue(Name, out IntPtr Script) ? Script : IntPtr.Zero;
    }
}
