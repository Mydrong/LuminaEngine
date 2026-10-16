namespace LuminaSharp;

/// <summary>
/// Global logging into the engine log, usable from anywhere (the C# analog of Unity's Debug). The
/// first of the global engine API classes; Time/Input/Physics/etc. follow the same pattern.
/// </summary>
public static class Debug
{
    public static void Log(string Message)
    {
        Native.Log(ELogLevel.Info, Message);
    }

    public static void Log(ref LogMessage<InfoLevel> Message)
    {
        Write(ELogLevel.Info, ref Message);
    }

    public static void LogWarning(string Message)
    {
        Native.Log(ELogLevel.Warn, Message);
    }

    public static void LogWarning(ref LogMessage<WarnLevel> Message)
    {
        Write(ELogLevel.Warn, ref Message);
    }

    public static void LogError(string Message)
    {
        Native.Log(ELogLevel.Error, Message);
    }

    public static void LogError(ref LogMessage<ErrorLevel> Message)
    {
        Write(ELogLevel.Error, ref Message);
    }

    private static void Write<TLevel>(ELogLevel Level, ref LogMessage<TLevel> Message) where TLevel : ILogLevel
    {
        Native.Log(Level, Message.Text());
        Message.Release();
    }
}
