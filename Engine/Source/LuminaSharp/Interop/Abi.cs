using System;
using System.Runtime.InteropServices;

namespace LuminaSharp;

// Log severity, mirroring the levels Export_Log switches on.
public enum ELogLevel
{
    Trace = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
}

// The native function pointers handed over at bootstrap, mirroring Lumina::DotNet::FExporterTable.
[StructLayout(LayoutKind.Sequential)]
public unsafe struct FExporterTable
{
    public delegate* unmanaged[Stdcall]<int, byte*, int, void> Log;
}

// The handshake payload Host.Bootstrap receives, mirroring Lumina::DotNet::FBootstrapArgs. Managed exports are resolved by name instead.
[StructLayout(LayoutKind.Sequential)]
public unsafe struct FBootstrapArgs
{
    public int AbiVersion;
    public FExporterTable* Exports;
    public IntPtr NativeModule;
}

// One script source read through the engine VFS, mirroring Lumina::DotNet::FSourceFile.
[StructLayout(LayoutKind.Sequential)]
public unsafe struct FSourceFile
{
    public byte* Path;
    public int PathLength;
    public byte* Text;
    public int TextLength;
}

// One compilation unit, mirroring Lumina::DotNet::FSourceAssembly, a prebuilt DllPath standing in for sources and Deps naming its siblings.
[StructLayout(LayoutKind.Sequential)]
public unsafe struct FSourceAssembly
{
    public byte* Name;
    public int NameLength;
    public byte* Deps;          // ';'-joined dependency unit names ("" if none)
    public int DepsLength;
    public FSourceFile* Sources;
    public int SourceCount;
    public byte* DllPath;       // optional prebuilt managed DLL (used when SourceCount == 0); "" otherwise
    public int DllPathLength;
    public byte* References;    // ';'-joined absolute paths to third-party assemblies ("" if none)
    public int ReferencesLength;
}
