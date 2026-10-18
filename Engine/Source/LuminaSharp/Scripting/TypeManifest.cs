using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

namespace LuminaSharp;

// Every type one script generation publishes, written as one blob so native learns the whole generation in one crossing.
internal static class TypeManifest
{
    // Mirrors Lumina::Scripting::kTypeManifestMagic, 'LTMF'.
    private const uint Magic = 0x464D544Cu;
    private const ushort Version = 2;

    // Mirrors Lumina::Scripting::EManagedTypeKind.
    private const byte ScriptableClass = 1;
    private const byte ScriptStruct = 2;
    private const byte ScriptEnum = 3;

    private const byte AbstractFlag = 1;

    public static byte[] Write(TypeLibrary Library, ScriptableRuntime Scriptables)
    {
        using var Stream = new MemoryStream();
        using var Writer = new BinaryWriter(Stream, Encoding.UTF8, leaveOpen: true);
        Writer.Write(Magic);
        Writer.Write(Version);

        // Parents first, since a class is minted under its C# parent and stacks its block after that parent's.
        List<Type> Classes = Library.ScriptableTypes.OrderBy(Depth).Where(Type => Scriptables.NativeBaseOf(Type) != null).ToList();
        List<KeyValuePair<string, ScriptStructEntry>> Structs = Library.ScriptStructTypes.ToList();
        List<KeyValuePair<string, Type>> Enums = Library.EnumTypes.ToList();
        Writer.Write(Classes.Count + Structs.Count + Enums.Count);

        foreach (KeyValuePair<string, Type> Pair in Enums)
        {
            List<KeyValuePair<string, string>> Meta = ReflectSpecifiers(Pair.Value);
            // A C# flags enum is what REFLECT(BitMask) declares in C++.
            if (Pair.Value.IsDefined(typeof(FlagsAttribute), inherit: false))
            {
                Meta.Add(new("BitMask", ""));
            }
            WriteHeader(Writer, ScriptEnum, Pair.Key, "", 0, Array.Empty<string>(), Meta);
            Type Underlying = Enum.GetUnderlyingType(Pair.Value);
            Writer.Write((byte)TypeLibrary.MapNumeric(Underlying));
            string[] Names = Enum.GetNames(Pair.Value);
            Array Values = Enum.GetValues(Pair.Value);
            Writer.Write(Names.Length);
            for (int Index = 0; Index < Names.Length; ++Index)
            {
                Serializer.WriteString(Writer, Names[Index]);
                // Bit preserving, so a ulong-backed member with its high bit set survives the trip.
                object Raw = Convert.ChangeType(Values.GetValue(Index)!, Underlying);
                Writer.Write(Raw is ulong Unsigned ? unchecked((long)Unsigned) : Convert.ToInt64(Raw));
            }
        }

        foreach (Type Class in Classes)
        {
            WriteType(Writer, ScriptableClass, NativeTypeName.Of(Class), Scriptables.NativeBaseOf(Class)!,
                (byte)(Class.IsAbstract ? AbstractFlag : 0), Scriptables.OverriddenEvents(Class), Scriptables.ClassMeta(Class),
                Serializer.WriteSchema(Library.Describe(Class), Library.MintedParentOf(Class)));
        }
        foreach (KeyValuePair<string, ScriptStructEntry> Pair in Structs)
        {
            List<KeyValuePair<string, string>> Meta = ReflectSpecifiers(Pair.Value.Description.Type);
            if (TypeLibrary.IsComponentStruct(Pair.Value.Description.Type))
            {
                Meta.Add(new("Component", ""));
            }
            string Implemented = TypeLibrary.ImplementedInterfaces(Pair.Value.Description.Type);
            if (Implemented.Length > 0)
            {
                Meta.Add(new("Implements", Implemented));
            }
            WriteType(Writer, ScriptStruct, Pair.Key, Pair.Value.NativeBase, 0, Array.Empty<string>(), Meta, Serializer.WriteSchema(Pair.Value.Description));
        }

        List<KeyValuePair<string, string>> Aliases = Library.TypeAliases.ToList();
        Writer.Write(Aliases.Count);
        foreach (KeyValuePair<string, string> Alias in Aliases)
        {
            Serializer.WriteString(Writer, Alias.Key);
            Serializer.WriteString(Writer, Alias.Value);
        }

        Writer.Flush();
        return Stream.ToArray();
    }

    private static void WriteType(BinaryWriter Writer, byte Kind, string Name, string NativeBase, byte Flags,
        IReadOnlyList<string> Overrides, IReadOnlyList<KeyValuePair<string, string>> Meta, byte[] Schema)
    {
        WriteHeader(Writer, Kind, Name, NativeBase, Flags, Overrides, Meta);
        Writer.Write(Schema.Length);
        Writer.Write(Schema);
    }

    private static void WriteHeader(BinaryWriter Writer, byte Kind, string Name, string NativeBase, byte Flags,
        IReadOnlyList<string> Overrides, IReadOnlyList<KeyValuePair<string, string>> Meta)
    {
        Writer.Write(Kind);
        Serializer.WriteString(Writer, Name);
        Serializer.WriteString(Writer, NativeBase);
        Writer.Write(Flags);
        Writer.Write(Overrides.Count);
        foreach (string Event in Overrides)
        {
            Serializer.WriteString(Writer, Event);
        }
        Writer.Write(Meta.Count);
        foreach (KeyValuePair<string, string> Pair in Meta)
        {
            Serializer.WriteString(Writer, Pair.Key);
            Serializer.WriteString(Writer, Pair.Value);
        }
    }

    private static List<KeyValuePair<string, string>> ReflectSpecifiers(Type Type)
    {
        return Type.GetCustomAttribute<ReflectAttribute>(inherit: false) is { } Reflect
            ? Reflect.Specifiers().ToList()
            : new List<KeyValuePair<string, string>>();
    }

    private static int Depth(Type Type)
    {
        int Count = 0;
        for (Type? Base = Type.BaseType; Base != null; Base = Base.BaseType)
        {
            ++Count;
        }
        return Count;
    }
}
