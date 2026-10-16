using LuminaBuildTool.Configuration;
using LuminaBuildTool.Core;

namespace LuminaBuildTool.Modes;

// Picks which engine checkout a project opens with, by id or path rather than version.
public static class SwitchEngineMode
{
    public static int Run(CommandLine Arguments, BuildDirectories Directories)
    {
        string? ProjectRoot = Directories.ProjectRoot;
        string? ProjectFile = ProjectRoot is null ? null : EngineAssociation.FindProjectFile(ProjectRoot);
        if (ProjectRoot is null || ProjectFile is null)
        {
            throw new BuildException("SwitchEngine requires a project file. Usage: LuminaBuildTool SwitchEngine <Path/To/Game.lproject> [-Engine=<id or path>]");
        }

        string Current = EngineAssociation.ReadAssociation(ProjectFile) ?? string.Empty;
        string? Choice = Arguments.GetString("Engine");

        if (Choice is null)
        {
            List<KeyValuePair<string, string>> Engines = EngineAssociation.LoadEngines()
                .OrderBy(Pair => Pair.Value, StringComparer.OrdinalIgnoreCase)
                .ToList();

            Log.Raw($"Engines for {Path.GetFileName(ProjectFile)}:");
            Log.Raw($"  0) Any, whichever engine opens it{(Current.Length == 0 ? "  (current)" : string.Empty)}");
            for (int Index = 0; Index < Engines.Count; ++Index)
            {
                bool bCurrent = string.Equals(Engines[Index].Key, Current, StringComparison.OrdinalIgnoreCase);
                Log.Raw($"  {Index + 1}) {Engines[Index].Value}{(bCurrent ? "  (current)" : string.Empty)}");
            }

            Console.Write("Choose an engine: ");
            if (!int.TryParse(Console.ReadLine(), out int Picked) || Picked < 0 || Picked > Engines.Count)
            {
                Log.Error("No engine was chosen, so {0} is unchanged.", Path.GetFileName(ProjectFile));
                return 1;
            }

            Choice = Picked == 0 ? string.Empty : Engines[Picked - 1].Key;
        }
        else if (Choice.Length > 0 && EngineAssociation.Resolve(Choice, ProjectRoot) is null)
        {
            throw new BuildException($"'{Choice}' is neither an engine id on this machine nor an engine root.");
        }

        EngineAssociation.WriteAssociation(ProjectFile, Choice);

        if (Choice.Length == 0)
        {
            Log.Info("{0} now opens with whichever engine is asked to open it.", Path.GetFileName(ProjectFile));
        }
        else
        {
            Log.Info("{0} now opens with the engine at {1}.", Path.GetFileName(ProjectFile), EngineAssociation.Resolve(Choice, ProjectRoot)!);
        }

        return 0;
    }
}
