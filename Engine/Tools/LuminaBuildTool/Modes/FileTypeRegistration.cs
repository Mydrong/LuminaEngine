using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security;
using LuminaBuildTool.Core;
using Microsoft.Win32;

namespace LuminaBuildTool.Modes;

// Gives the engine's descriptor files the Lumina icon and project verbs, per user so no elevation is needed.
public static class FileTypeRegistration
{
    private static readonly (string Extension, string ProgId, string Description)[] FileTypes =
    {
        (".lproject", "Lumina.Project", "Lumina Project"),
        (".lplugin", "Lumina.Plugin", "Lumina Plugin"),
    };

    // A failed command keeps its window open so the errors can be read, and Switch Engine always does to show the result.
    private static readonly (string Key, string Label, string Arguments, string AfterRun)[] ProjectVerbs =
    {
        ("LuminaOpen", "Open Project", "Open \"%1\"", "|| pause"),
        ("LuminaLaunch", "Launch Game", "Launch \"%1\"", "|| pause"),
        ("LuminaGenerate", "Generate Project Files", "GenerateProjectFiles -Project=\"%1\"", "|| pause"),
        ("LuminaSwitchEngine", "Switch Engine...", "SwitchEngine \"%1\"", "& pause"),
    };

    private const string ProjectVerbsKey = @"Software\Classes\SystemFileAssociations\.lproject\shell";

    private const int ShellAssociationChanged = 0x08000000;

    // Repairs only what is missing or points at a deleted engine, so two checkouts never take turns rewriting it.
    public static void EnsureRegistered(string EngineRoot)
    {
        if (OperatingSystem.IsWindows() && !IsRegistrationIntact())
        {
            Register(EngineRoot);
        }
    }

    [SupportedOSPlatform("windows")]
    private static bool IsRegistrationIntact()
    {
        foreach ((string _, string ProgId, string _) in FileTypes)
        {
            using RegistryKey? IconKey = Registry.CurrentUser.OpenSubKey($@"Software\Classes\{ProgId}\DefaultIcon");
            if (IconKey?.GetValue(string.Empty) is not string Icon || !File.Exists(Icon.Split(',')[0].Trim('"')))
            {
                return false;
            }
        }

        foreach ((string Key, string _, string _, string _) in ProjectVerbs)
        {
            using RegistryKey? CommandKey = Registry.CurrentUser.OpenSubKey($@"{ProjectVerbsKey}\{Key}\command");
            string[] Quoted = (CommandKey?.GetValue(string.Empty) as string ?? string.Empty).Split('"', StringSplitOptions.RemoveEmptyEntries);
            if (Quoted.Length < 2 || !File.Exists(Quoted[1]))
            {
                return false;
            }
        }

        return true;
    }

    public static void Register(string EngineRoot)
    {
        if (!OperatingSystem.IsWindows())
        {
            Log.Info("File type icons are an Explorer feature, so there is nothing to register on this platform.");
            return;
        }

        string IconPath = Path.Combine(EngineRoot, "Engine", "Resources", "Icons", "Lumina.ico");
        if (!File.Exists(IconPath))
        {
            Log.Warning("{0} is missing, so .lproject and .lplugin keep their current icon.", IconPath);
            return;
        }

        try
        {
            WriteClasses(IconPath);
            WriteProjectVerbs(IconPath, Path.GetFullPath(Path.Combine(EngineRoot, "LuminaBuild.bat")));
        }
        catch (Exception Ex) when (Ex is SecurityException or UnauthorizedAccessException or IOException)
        {
            Log.Warning("Could not register the Lumina file icons: {0}", Ex.Message);
        }
    }

    [SupportedOSPlatform("windows")]
    private static void WriteClasses(string IconPath)
    {
        using RegistryKey Classes = Registry.CurrentUser.CreateSubKey(@"Software\Classes");
        foreach ((string Extension, string ProgId, string Description) in FileTypes)
        {
            using (RegistryKey ExtensionKey = Classes.CreateSubKey(Extension))
            {
                ExtensionKey.SetValue(string.Empty, ProgId);
            }

            using RegistryKey TypeKey = Classes.CreateSubKey(ProgId);
            TypeKey.SetValue(string.Empty, Description);
            using RegistryKey IconKey = TypeKey.CreateSubKey("DefaultIcon");
            IconKey.SetValue(string.Empty, $"\"{IconPath}\",0");

            // An app picked through Open With outranks the type, and Windows guards that choice, so it is only reported.
            using RegistryKey? Choice = Registry.CurrentUser.OpenSubKey($@"Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\{Extension}\UserChoice");
            if (Choice?.GetValue("ProgId") is string Chosen && Chosen != ProgId)
            {
                Log.Warning("{0} files open with {1}, whose icon Explorer shows instead. Clear that choice in Settings > Apps > Default apps to see the Lumina icon.", Extension, Chosen);
                continue;
            }

            Log.Info("{0} files show the Lumina icon.", Extension);
        }

        // Explorer caches icons per extension and only drops them when told an association changed.
        SHChangeNotify(ShellAssociationChanged, 0, IntPtr.Zero, IntPtr.Zero);
    }

    // Keyed by extension rather than our type, so they show even when another app owns double-click.
    [SupportedOSPlatform("windows")]
    private static void WriteProjectVerbs(string IconPath, string BuildScript)
    {
        foreach ((string Key, string Label, string Arguments, string AfterRun) in ProjectVerbs)
        {
            using RegistryKey Verb = Registry.CurrentUser.CreateSubKey($@"{ProjectVerbsKey}\{Key}");
            Verb.SetValue(string.Empty, Label);
            Verb.SetValue("Icon", $"\"{IconPath}\",0");

            using RegistryKey Command = Verb.CreateSubKey("command");
            Command.SetValue(string.Empty, $"cmd.exe /c \"\"{BuildScript}\" {Arguments} {AfterRun}\"");
        }

        Log.Info("Right-clicking a .lproject offers {0}.", string.Join(", ", ProjectVerbs.Select(Verb => Verb.Label.TrimEnd('.'))));
    }

    [DllImport("shell32.dll")]
    private static extern void SHChangeNotify(int EventId, uint Flags, IntPtr Item1, IntPtr Item2);
}
