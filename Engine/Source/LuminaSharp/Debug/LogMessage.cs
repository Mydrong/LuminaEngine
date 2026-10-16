using System;
using System.Buffers;
using System.Runtime.CompilerServices;
using System.Text;

namespace LuminaSharp;

// Names a log level as a type, so an interpolated handler knows its level before any hole is evaluated.
public interface ILogLevel
{
    static abstract ELogLevel Level { get; }
}

public readonly struct InfoLevel : ILogLevel { public static ELogLevel Level => ELogLevel.Info; }
public readonly struct WarnLevel : ILogLevel { public static ELogLevel Level => ELogLevel.Warn; }
public readonly struct ErrorLevel : ILogLevel { public static ELogLevel Level => ELogLevel.Error; }

// Formats straight to UTF-8 in a pooled buffer, and below the engine's threshold skips every hole, so a filtered log costs one byte read.
[InterpolatedStringHandler]
public ref struct LogMessage<TLevel> where TLevel : ILogLevel
{
    private const int MinimumCapacity = 256;

    private byte[]? Buffer;
    private int Length;

    public LogMessage(int LiteralLength, int FormattedCount, out bool bEnabled)
    {
        bEnabled = Native.IsLogEnabled(TLevel.Level);
        Buffer = bEnabled ? ArrayPool<byte>.Shared.Rent(Math.Max(MinimumCapacity, LiteralLength * 3 + FormattedCount * 16)) : null;
        Length = 0;
    }

    public void AppendLiteral(string Text)
    {
        AppendChars(Text);
    }

    public void AppendFormatted(string? Value)
    {
        AppendChars(Value);
    }

    public void AppendFormatted(ReadOnlySpan<char> Value)
    {
        AppendChars(Value);
    }

    public void AppendFormatted<T>(T Value)
    {
        AppendFormatted(Value, 0, null);
    }

    // The compiler binds a hole's ",width" and ":format" by these exact parameter names.
    public void AppendFormatted<T>(T Value, string? format)
    {
        AppendFormatted(Value, 0, format);
    }

    public void AppendFormatted<T>(T Value, int alignment)
    {
        AppendFormatted(Value, alignment, null);
    }

    public void AppendFormatted<T>(T Value, int alignment, string? format)
    {
        int Start = Length;
        if (Value is IUtf8SpanFormattable)
        {
            int Written;
            while (!((IUtf8SpanFormattable)Value).TryFormat(Buffer.AsSpan(Length), out Written, format, null))
            {
                Grow(Buffer!.Length * 2);
            }
            Length += Written;
        }
        else
        {
            AppendChars(Value is IFormattable ? ((IFormattable)Value).ToString(format, null) : Value?.ToString());
        }

        if (alignment != 0)
        {
            Pad(Start, alignment);
        }
    }

    // A positive width pads on the left and a negative one on the right, counted in characters as string.Format does.
    private void Pad(int Start, int Alignment)
    {
        int Width = Math.Abs(Alignment);
        int Characters = Encoding.UTF8.GetCharCount(Buffer.AsSpan(Start, Length - Start));
        int Spaces = Width - Characters;
        if (Spaces <= 0)
        {
            return;
        }

        if (Length + Spaces > Buffer!.Length)
        {
            Grow(Length + Spaces);
        }
        if (Alignment > 0)
        {
            Buffer.AsSpan(Start, Length - Start).CopyTo(Buffer.AsSpan(Start + Spaces));
            Buffer.AsSpan(Start, Spaces).Fill((byte)' ');
        }
        else
        {
            Buffer.AsSpan(Length, Spaces).Fill((byte)' ');
        }
        Length += Spaces;
    }

    internal ReadOnlySpan<byte> Text()
    {
        return Buffer.AsSpan(0, Length);
    }

    internal void Release()
    {
        if (Buffer != null)
        {
            ArrayPool<byte>.Shared.Return(Buffer);
            Buffer = null;
        }
    }

    private void AppendChars(ReadOnlySpan<char> Text)
    {
        int Needed = Length + Encoding.UTF8.GetMaxByteCount(Text.Length);
        if (Needed > Buffer!.Length)
        {
            Grow(Needed);
        }
        Length += Encoding.UTF8.GetBytes(Text, Buffer.AsSpan(Length));
    }

    private void Grow(int Capacity)
    {
        byte[] Larger = ArrayPool<byte>.Shared.Rent(Capacity);
        Buffer.AsSpan(0, Length).CopyTo(Larger);
        ArrayPool<byte>.Shared.Return(Buffer!);
        Buffer = Larger;
    }
}
