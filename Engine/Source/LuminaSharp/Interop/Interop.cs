using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

// Blittable-only boundary: no runtime marshaling on any [UnmanagedCallersOnly] entry.
[assembly: DisableRuntimeMarshalling]

namespace LuminaSharp;

// String transfer and exception containment shared by every crossing.
internal static unsafe class Interop
{
    // Longer than this => treated as corrupt rather than read (guards a bad pointer becoming a huge allocation).
    private const int MaxNativeStringBytes = 64 * 1024 * 1024;

    /// <summary>Reads a counted UTF-8 buffer from native into a managed string. Null/empty -&gt; "".</summary>
    public static string GetString(byte* Pointer, int ByteLength)
    {
        if (Pointer == null || ByteLength <= 0)
        {
            return string.Empty;
        }
        if (ByteLength >= MaxNativeStringBytes)
        {
            Native.Log(ELogLevel.Warn, "Interop.GetString: refusing to read an implausibly long native string.");
            return string.Empty;
        }
        return Encoding.UTF8.GetString(Pointer, ByteLength);
    }

    /// <summary>Reads a NUL-terminated UTF-8 string from native. Null -&gt; "".</summary>
    public static string GetString(byte* Pointer)
    {
        if (Pointer == null)
        {
            return string.Empty;
        }

        ReadOnlySpan<byte> Span = MemoryMarshal.CreateReadOnlySpanFromNullTerminated(Pointer);
        if (Span.Length >= MaxNativeStringBytes)
        {
            Native.Log(ELogLevel.Warn, "Interop.GetString: refusing to read an implausibly long native string.");
            return string.Empty;
        }

        return Encoding.UTF8.GetString(Span);
    }

    // UTF-8 for one native call, NUL-terminated, written into the caller's stack scratch whenever it fits.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static byte* ToUtf8(string? Value, byte* Scratch, int Capacity, out int Length)
    {
        // UTF-8 never takes more than three bytes per UTF-16 unit, so this bound needs no counting pass.
        if (Value != null && Value.Length * 3 < Capacity)
        {
            Length = Encoding.UTF8.GetBytes(Value, new Span<byte>(Scratch, Capacity));
            Scratch[Length] = 0;
            return Scratch;
        }
        return ToUtf8Slow(Value, Scratch, Capacity, out Length);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static byte* ToUtf8Slow(string? Value, byte* Scratch, int Capacity, out int Length)
    {
        Length = 0;
        if (Value == null)
        {
            return null;
        }

        Length = Encoding.UTF8.GetByteCount(Value);
        byte* Target = Length < Capacity ? Scratch : (byte*)NativeMemory.Alloc((nuint)Length + 1);
        fixed (char* Source = Value)
        {
            Encoding.UTF8.GetBytes(Source, Value.Length, Target, Length);
        }
        Target[Length] = 0;
        return Target;
    }

    // Only a string too long for the scratch went to the heap.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void FreeUtf8(byte* Encoded, byte* Scratch)
    {
        if (Encoded != Scratch && Encoded != null)
        {
            NativeMemory.Free(Encoded);
        }
    }

    // ToUtf8 over a span, for hand-written crossings. Always Free it in a finally.
    public unsafe ref struct FInteropString
    {
        public byte* Pointer;
        public int Length;
        private readonly byte* Scratch;

        public FInteropString(string Value, Span<byte> ScratchBuffer)
        {
            Scratch = (byte*)Unsafe.AsPointer(ref MemoryMarshal.GetReference(ScratchBuffer));
            Pointer = ToUtf8(Value, Scratch, ScratchBuffer.Length, out Length);
        }

        public void Free()
        {
            FreeUtf8(Pointer, Scratch);
            Pointer = null;
        }
    }

    // Logs an exception caught at a native boundary, so a managed exception never unwinds into native code.
    public static void LogException(Exception Exception)
    {
        LogException(Exception, null);
    }

    // Keyed by throw site, so a script throwing every frame logs its stack once and then a count.
    private static readonly Dictionary<string, int> ExceptionCounts = new();
    private const int MaxTrackedExceptionSites = 4096;

    internal static void LogException(Exception Exception, string? Context)
    {
        try
        {
            string Where = Context != null ? " in " + Context : string.Empty;
            string Key = Exception.GetType().FullName + "@" + Exception.TargetSite?.DeclaringType?.FullName + "." + Exception.TargetSite?.Name + Where;

            int Count;
            lock (ExceptionCounts)
            {
                if (ExceptionCounts.Count >= MaxTrackedExceptionSites && !ExceptionCounts.ContainsKey(Key))
                {
                    ExceptionCounts.Clear();
                }
                ExceptionCounts.TryGetValue(Key, out Count);
                ExceptionCounts[Key] = ++Count;
            }

            if (Count == 1)
            {
                Native.Log(ELogLevel.Error, "LuminaSharp boundary exception" + Where + ": " + Exception);
            }
            else if (IsPowerOfTen(Count))
            {
                Native.Log(ELogLevel.Error, $"LuminaSharp boundary exception{Where} repeated {Count} times: {Exception.GetType().Name}: {Exception.Message}");
            }
        }
        catch
        {
            // The boundary must not throw, even if logging fails.
        }
    }

    // A reload usually means the throwing code changed, so its first throw deserves a full stack again.
    internal static void ResetExceptionThrottle()
    {
        lock (ExceptionCounts)
        {
            ExceptionCounts.Clear();
        }
    }

    private static bool IsPowerOfTen(int Value)
    {
        while (Value >= 10 && Value % 10 == 0)
        {
            Value /= 10;
        }
        return Value == 1;
    }
}
