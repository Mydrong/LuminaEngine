using System.Collections.Immutable;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace LuminaSharp.Generators;

// The engine registers only a class that carries [Reflect], so a forgotten one is flagged where it is written instead of in the editor log.
[Generator]
public sealed class ScriptClassReflectAnalyzer : IIncrementalGenerator
{
    private const string ReflectAttribute = "LuminaSharp.ReflectAttribute";
    private const string NativeTypeAttribute = "LuminaSharp.NativeTypeAttribute";

    private static readonly DiagnosticDescriptor MissingReflect = new(
        id: "LUM0112",
        title: "Script class needs [Reflect]",
        messageFormat: "'{0}' derives from the engine class '{1}' but has no [Reflect], so the engine never registers it. Add [Reflect], or make it abstract to keep it a plain C# base.",
        category: "LuminaSharp",
        defaultSeverity: DiagnosticSeverity.Warning,
        isEnabledByDefault: true);

    public void Initialize(IncrementalGeneratorInitializationContext Context)
    {
        IncrementalValueProvider<ImmutableArray<INamedTypeSymbol?>> Classes = Context.SyntaxProvider
            .CreateSyntaxProvider(
                static (Node, _) => Node is ClassDeclarationSyntax { BaseList: not null },
                static (Syntax, _) => Syntax.SemanticModel.GetDeclaredSymbol((ClassDeclarationSyntax)Syntax.Node) as INamedTypeSymbol)
            .Where(static Symbol => Symbol != null && !Symbol.IsAbstract && NativeBaseOf(Symbol) != null && !HasAttribute(Symbol, ReflectAttribute))
            .Collect();

        Context.RegisterSourceOutput(Classes, static (Output, Symbols) =>
        {
            foreach (INamedTypeSymbol Symbol in Symbols.Distinct(SymbolEqualityComparer.Default).Cast<INamedTypeSymbol>())
            {
                Output.ReportDiagnostic(Diagnostic.Create(MissingReflect, Symbol.Locations.FirstOrDefault(), Symbol.Name, NativeBaseOf(Symbol)!.Name));
            }
        });
    }

    private static bool HasAttribute(INamedTypeSymbol Symbol, string Attribute)
    {
        return Symbol.GetAttributes().Any(Data => Data.AttributeClass?.ToDisplayString() == Attribute);
    }

    // The nearest generated wrapper above Symbol, mirroring TypeLibrary.NativeClassBaseOf at runtime.
    private static INamedTypeSymbol? NativeBaseOf(INamedTypeSymbol Symbol)
    {
        if (HasAttribute(Symbol, NativeTypeAttribute))
        {
            return null;
        }
        for (INamedTypeSymbol? Base = Symbol.BaseType; Base != null; Base = Base.BaseType)
        {
            if (HasAttribute(Base, NativeTypeAttribute))
            {
                return Base;
            }
        }
        return null;
    }
}
