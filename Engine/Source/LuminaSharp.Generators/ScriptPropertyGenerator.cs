using System.Linq;
using LuminaSharp.ScriptProperties;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace LuminaSharp.Generators;

/// <summary>
/// Validates <c>[Property]</c> members. It emits nothing.
///
/// The accessors themselves are produced by <c>LuminaSharp.ScriptPropertyRewriter</c>, inside the engine's own
/// Roslyn compilation, because a script property's value lives in NATIVE memory and a field cannot be a view
/// over that -- only an accessor can. A source generator may only ADD members, never replace one, which is
/// what used to force authors to write <c>public partial float Speed { get; set; }</c> and left nowhere to put
/// a default (a partial property cannot have an initializer). The engine compiles scripts itself, so it
/// rewrites the field into that property instead, and the author writes ordinary C#.
///
/// This analyzer still exists because the rewriter runs at RUN TIME: without it an unsupported member would
/// look fine in the IDE and fail only on reload. Its job is to say the same thing the rewriter would, at the
/// declaration, while you are typing it -- so it does not decide anything itself. Every judgement here comes
/// from <see cref="ScriptPropertyClassifier"/>, the single copy the rewriter also runs on.
/// </summary>
[Generator]
public sealed class ScriptPropertyGenerator : IIncrementalGenerator
{
    private const string PropertyAttribute = "LuminaSharp.PropertyAttribute";
    private const string NativeObjectMetadataName = "LuminaSharp.NativeObject";

    private static readonly DiagnosticDescriptor UnsupportedType = new(
        id: "LUM0101",
        title: "Unsupported [Property] type",
        messageFormat: "[Property] '{0}' has type '{1}': {2}",
        category: "LuminaSharp",
        defaultSeverity: DiagnosticSeverity.Error,
        isEnabledByDefault: true);

    private static readonly DiagnosticDescriptor ContainerInitialized = new(
        id: "LUM0102",
        title: "Container [Property] cannot be initialized",
        messageFormat: "[Property] '{0}' is a container, so it is a VIEW over storage native already owns and cannot be "
                     + "assigned. Drop the initializer and fill it in OnReady (Add/Clear/RemoveAt).",
        category: "LuminaSharp",
        defaultSeverity: DiagnosticSeverity.Error,
        isEnabledByDefault: true);

    private static readonly DiagnosticDescriptor PartialProperty = new(
        id: "LUM0103",
        title: "[Property] should be a field",
        messageFormat: "[Property] '{0}' is a partial property. That was the old shape, needed back when a source "
                     + "generator produced the accessors; the engine now rewrites a plain field into the same thing. "
                     + "Declare it as 'public {1} {0};' -- which also gives an initializer somewhere to live, and that "
                     + "is what a default is.",
        category: "LuminaSharp",
        defaultSeverity: DiagnosticSeverity.Error,
        isEnabledByDefault: true);

    private static readonly DiagnosticDescriptor StoredPropertyShape = new(
        id: "LUM0104",
        title: "Stored property must be an auto-property",
        messageFormat: "'{0}' {1}",
        category: "LuminaSharp",
        defaultSeverity: DiagnosticSeverity.Error,
        isEnabledByDefault: true);

    // Every attribute ScriptMemberRules stores a field for, so a bad [SaveGame] type is flagged as early as a bad [Property].
    private static readonly string[] StorageAttributes =
    {
        PropertyAttribute, "LuminaSharp.SerializeAttribute", "LuminaSharp.SyncAttribute", "LuminaSharp.SaveGameAttribute",
        "LuminaSharp.BindAttribute",
    };

    public void Initialize(IncrementalGeneratorInitializationContext context)
    {
        foreach (string Attribute in StorageAttributes)
        {
            IncrementalValuesProvider<Diagnostic?> Diagnostics = context.SyntaxProvider
                .ForAttributeWithMetadataName(
                    Attribute,
                    static (_, _) => true,
                    (Context, _) => ReportsFor(Context.TargetSymbol, Attribute)
                        ? Validate(Context.TargetSymbol, Context.TargetNode, Context.SemanticModel.Compilation)
                        : null);

            context.RegisterSourceOutput(Diagnostics, static (Context, Item) =>
            {
                if (Item != null)
                {
                    Context.ReportDiagnostic(Item);
                }
            });
        }
    }

    private static Diagnostic? Validate(ISymbol Symbol, SyntaxNode Node, Compilation Compilation)
    {
        Location Location = Node.GetLocation();
        bool bNativeOwner = DerivesFrom(Symbol.ContainingType, Compilation.GetTypeByMetadataName(NativeObjectMetadataName));

        // A partial property is the pre-rewriter shape. It still compiles in the IDE (nothing implements it
        // there either), so without this it would fail only at script reload, with CS9248 and no explanation.
        if (Symbol is IPropertySymbol Property)
        {
            if (IsPartial(Property))
            {
                return Diagnostic.Create(PartialProperty, Symbol.Locations.FirstOrDefault() ?? Location,
                    Property.Name, Property.Type.ToDisplayString());
            }
            return ValidateProperty(Property, bNativeOwner, Location);
        }

        if (Symbol is not IFieldSymbol Field || !ScriptPropertyClassifier.RuleFor(Field, EScriptMemberKind.Field, bNativeOwner).bStored)
        {
            return null;
        }

        FScriptPropertyClassification Classification = ScriptPropertyClassifier.Classify(Field.Type);
        if (!Classification.IsSupported)
        {
            return Diagnostic.Create(UnsupportedType, Location, Field.Name, Field.Type.ToDisplayString(),
                Classification.Rejection);
        }

        // A container is a view over storage native owns, so assigning it is meaningless while its contents
        // are fully editable. The rewriter refuses this too; reported here so it surfaces at the declaration.
        if (Classification.IsView && Declarator(Field, Node)?.Initializer != null)
        {
            return Diagnostic.Create(ContainerInitialized, Location, Field.Name);
        }

        return null;
    }

    // The rewriter backs a script's stored auto-property with native storage, so any other shape would edit a copy native never sees.
    private static Diagnostic? ValidateProperty(IPropertySymbol Property, bool bNativeOwner, Location Location)
    {
        if (!bNativeOwner || Property.IsStatic || IsNativeBackedAlready(Property))
        {
            return null;
        }

        FScriptPropertyClassification Classification = ScriptPropertyClassifier.Classify(Property.Type);
        EScriptMemberKind Kind = Property.SetMethod != null || Classification.IsView ? EScriptMemberKind.WritableProperty : EScriptMemberKind.ReadOnlyProperty;
        if (!ScriptPropertyClassifier.RuleFor(Property, Kind, bOwnerIsNativeObject: true).bStored)
        {
            return null;
        }

        PropertyDeclarationSyntax? Declaration = Property.DeclaringSyntaxReferences
            .Select(Reference => Reference.GetSyntax())
            .OfType<PropertyDeclarationSyntax>()
            .FirstOrDefault();
        if (Declaration == null)
        {
            return null;
        }

        string? Shape = null;
        if (Property.IsAbstract)
        {
            Shape = "is abstract, and a stored property needs storage of its own. Declare it on the class that holds the value.";
        }
        else if (Declaration.AccessorList == null
                 || Declaration.AccessorList.Accessors.Any(Accessor => Accessor.Body != null || Accessor.ExpressionBody != null))
        {
            Shape = "has accessor bodies, but the engine keeps a stored property's value in native memory, so a body would read and "
                  + "write a copy the inspector, saves and prefabs never see. Declare it as '{ get; set; }' or as a field.";
        }
        else if (Declaration.AccessorList.Accessors.Any(Accessor => Accessor.IsKind(SyntaxKind.InitAccessorDeclaration)))
        {
            Shape = "has an init accessor, but the engine assigns a stored value after construction. Use 'private set' to keep it read-only to callers.";
        }
        if (Shape != null)
        {
            return Diagnostic.Create(StoredPropertyShape, Property.Locations.FirstOrDefault() ?? Location, Property.Name, Shape);
        }

        if (!Classification.IsSupported)
        {
            return Diagnostic.Create(UnsupportedType, Location, Property.Name, Property.Type.ToDisplayString(), Classification.Rejection);
        }
        if (Classification.IsView && Declaration.Initializer != null)
        {
            return Diagnostic.Create(ContainerInitialized, Location, Property.Name);
        }
        return null;
    }

    // LuminaSharp writes its own native accessors by hand, and the rewriter's output keeps its lazy offset or token beside the property.
    private static bool IsNativeBackedAlready(IPropertySymbol Property)
    {
        if (Property.ContainingAssembly?.Name == "LuminaSharp")
        {
            return true;
        }
        INamedTypeSymbol Owner = Property.ContainingType;
        return !Owner.GetMembers("__lazyoff_" + Property.Name).IsEmpty || !Owner.GetMembers("__lazyprop_" + Property.Name).IsEmpty;
    }

    private static bool DerivesFrom(INamedTypeSymbol? Type, INamedTypeSymbol? Base)
    {
        for (INamedTypeSymbol? Current = Type; Current != null && Base != null; Current = Current.BaseType)
        {
            if (SymbolEqualityComparer.Default.Equals(Current, Base))
            {
                return true;
            }
        }
        return false;
    }

    // One member carrying two storage attributes is reported once, by whichever comes first in the list.
    private static bool ReportsFor(ISymbol Symbol, string Attribute)
    {
        foreach (string Candidate in StorageAttributes)
        {
            if (Symbol.GetAttributes().Any(Data => Data.AttributeClass?.ToDisplayString() == Candidate))
            {
                return Candidate == Attribute;
            }
        }
        return false;
    }

    private static bool IsPartial(IPropertySymbol Property)
    {
        foreach (SyntaxReference Reference in Property.DeclaringSyntaxReferences)
        {
            if (Reference.GetSyntax() is PropertyDeclarationSyntax Declaration
                && Declaration.Modifiers.Any(SyntaxKind.PartialKeyword))
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>The declarator for a field, which is where an initializer lives -- an <see cref="IFieldSymbol"/>
    /// cannot see one. Normally the attribute's own target node; falls back through the symbol for the shapes
    /// where it is not (several declarators sharing one <c>[Property]</c> field declaration).</summary>
    private static VariableDeclaratorSyntax? Declarator(IFieldSymbol Field, SyntaxNode Node)
    {
        if (Node is VariableDeclaratorSyntax Direct)
        {
            return Direct;
        }
        return Field.DeclaringSyntaxReferences
            .Select(Reference => Reference.GetSyntax())
            .OfType<VariableDeclaratorSyntax>()
            .FirstOrDefault();
    }
}
