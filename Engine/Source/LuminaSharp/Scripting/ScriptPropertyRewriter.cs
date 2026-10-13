using System.Collections.Generic;
using System.Linq;
using System.Text;
using LuminaSharp.ScriptProperties;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace LuminaSharp;

/// <summary>
/// Rewrites <c>[Property] public float Speed = 5.0f;</c> into a property whose accessors read and write the
/// NATIVE storage the engine minted for that member, and moves the initializer to where a default belongs.
///
/// Why a rewriter and not a source generator. A script property's value has to live in native memory: it is
/// what the inspector, the tagged serializer, undo, prefab overrides and replication all read, and they read
/// it through a pointer at <c>Container + Offset</c> while the managed instance may not even exist yet. So the
/// C# side must go through that pointer, which a field cannot do -- only an accessor can. A source generator
/// may only ADD members, never replace one, which is what forced the author to write
/// <c>public partial float Speed { get; set; }</c> and left nowhere to put a default (a partial property may
/// not have an initializer).
///
/// None of that constrains US, because the engine compiles scripts itself: <see cref="ScriptCompiler"/> owns
/// the syntax trees before they are compiled, and the assembly it emits is the only one ever loaded -- the
/// generated .csproj is IntelliSense-only, and packaging stages this compiler's output. So the field is simply
/// turned into the property it needs to be, and the author writes ordinary C#.
///
/// The initializer becomes a default rather than a constructor store. A constructor store would run when the
/// managed wrapper is created, which is AFTER the native object has been loaded from a scene -- it would
/// overwrite every authored value with the declared default. Instead each class gets a generated
/// <c>__ApplyScriptDefaults</c> which the engine runs once against the class default object; every instance is
/// then copied from that, exactly as a C++ class gets its defaults from its constructor via its CDO.
/// </summary>
internal static class ScriptPropertyRewriter
{
    /// <summary>Rewrites one tree against a compilation used only to classify member types. Returns the tree
    /// unchanged when it declares no fields needing native storage.</summary>
    public static SyntaxTree Rewrite(CSharpCompilation Probe, SyntaxTree Tree, List<string> OutErrors)
    {
        SyntaxNode Root = Tree.GetRoot();
        if (!Root.DescendantNodes().OfType<FieldDeclarationSyntax>().Any(Field => MayNeedNativeStorage(Field.AttributeLists))
            && !Root.DescendantNodes().OfType<PropertyDeclarationSyntax>().Any(Property => MayNeedNativeStorage(Property.AttributeLists))
            && !Root.DescendantNodes().OfType<MethodDeclarationSyntax>().Any(MayBeRpc))
        {
            return Tree;
        }

        var Walker = new Rewriter(Probe.GetSemanticModel(Tree), Tree.FilePath, OutErrors);
        SyntaxNode Rewritten = Walker.Visit(Root);
        if (OutErrors.Count > 0)
        {
            return Tree;
        }

        // The original path is kept so #line directives (emitted per member) resolve, and so a diagnostic
        // Roslyn raises anywhere else in the file still names the file the author wrote.
        return CSharpSyntaxTree.Create((CSharpSyntaxNode)Rewritten, (CSharpParseOptions?)Tree.Options, Tree.FilePath, Encoding.UTF8);
    }

    // A syntax-only screen, so a file with no storage attribute anywhere costs no semantic model. ScriptMemberRules decides.
    private static bool MayNeedNativeStorage(SyntaxList<AttributeListSyntax> AttributeLists)
    {
        return AttributeLists
            .SelectMany(List => List.Attributes)
            .Any(Attribute => ScriptMemberRules.BareName(Attribute.Name.ToString()) is ScriptMemberRules.Property
                or ScriptMemberRules.Serialize or ScriptMemberRules.Sync or ScriptMemberRules.SaveGame or ScriptMemberRules.Bind);
    }

    private static bool HasAttributeNamed(SyntaxList<AttributeListSyntax> Lists, string Wanted)
    {
        return Lists.SelectMany(List => List.Attributes).Any(Attribute =>
        {
            string Name = Attribute.Name.ToString();
            int Dot = Name.LastIndexOf('.');
            if (Dot >= 0)
            {
                Name = Name.Substring(Dot + 1);
            }
            return Name == Wanted || Name == Wanted + "Attribute";
        });
    }

    // Syntax only, so the cheap pre-check costs no semantic model. The semantic pass confirms each one.
    private static bool MayBeRpc(MethodDeclarationSyntax Method)
    {
        return Method.AttributeLists.SelectMany(List => List.Attributes).Any(Attribute =>
        {
            string Name = Attribute.Name.ToString();
            return Name.Contains("Rpc.") || Name.Contains("RpcAttribute");
        });
    }

    private sealed class Rewriter : CSharpSyntaxRewriter
    {
        private readonly SemanticModel Model;
        private readonly string FilePath;
        private readonly List<string> Errors;
        private readonly INamedTypeSymbol? NativeObjectSymbol;
        private readonly INamedTypeSymbol? EntityScriptSymbol;

        public Rewriter(SemanticModel Model, string FilePath, List<string> Errors)
        {
            this.Model = Model;
            this.FilePath = FilePath;
            this.Errors = Errors;
            NativeObjectSymbol = Model.Compilation.GetTypeByMetadataName(typeof(NativeObject).FullName!);
            EntityScriptSymbol = Model.Compilation.GetTypeByMetadataName(typeof(Lumina.CEntityScript).FullName!);
        }

        public override SyntaxNode? VisitClassDeclaration(ClassDeclarationSyntax Node)
        {
            bool bNativeBacked = DerivesFrom(Model.GetDeclaredSymbol(Node), NativeObjectSymbol);
            List<FStoredMember> Stored = CollectStoredMembers(Node, bNativeBacked);
            List<FRpcMethod> Rpcs = CollectRpcs(Node);
            if (Stored.Count == 0 && Rpcs.Count == 0)
            {
                return base.VisitClassDeclaration(Node);
            }
            return RewriteClass(Node, Stored, Rpcs);
        }

        // One member whose value lives in native storage, a field declarator or an auto-property.
        private sealed class FStoredMember
        {
            public required MemberDeclarationSyntax Declaration;
            public required SyntaxNode At;
            public required ISymbol Symbol;
            public required ITypeSymbol Type;
            public required EqualsValueClauseSyntax? Initializer;
            public required bool bReadOnly;
            public string SetKeyword = "set";
        }

        private List<FStoredMember> CollectStoredMembers(ClassDeclarationSyntax Node, bool bNativeBacked)
        {
            var Result = new List<FStoredMember>();
            foreach (MemberDeclarationSyntax Member in Node.Members)
            {
                if (Member is FieldDeclarationSyntax Field && MayNeedNativeStorage(Field.AttributeLists) && IsStored(Field, bNativeBacked))
                {
                    foreach (VariableDeclaratorSyntax Declarator in Field.Declaration.Variables)
                    {
                        if (Model.GetDeclaredSymbol(Declarator) is IFieldSymbol Symbol)
                        {
                            Result.Add(new FStoredMember
                            {
                                Declaration = Field,
                                At = Declarator,
                                Symbol = Symbol,
                                Type = Symbol.Type,
                                Initializer = Declarator.Initializer,
                                bReadOnly = Symbol.IsReadOnly,
                            });
                        }
                    }
                }
                else if (Member is PropertyDeclarationSyntax Property && bNativeBacked && MayNeedNativeStorage(Property.AttributeLists)
                         && AsStoredProperty(Property) is { } StoredProperty)
                {
                    Result.Add(StoredProperty);
                }
            }
            return Result;
        }

        // Native minted storage for this property, so its accessors must reach it, or the inspector, saves and prefabs edit a value the script never reads.
        private FStoredMember? AsStoredProperty(PropertyDeclarationSyntax Property)
        {
            if (Model.GetDeclaredSymbol(Property) is not IPropertySymbol Symbol || Symbol.IsStatic
                || Property.Modifiers.Any(SyntaxKind.PartialKeyword))
            {
                return null;
            }

            bool bView = ScriptPropertyClassifier.Classify(Symbol.Type).IsView;
            EScriptMemberKind Kind = Symbol.SetMethod != null || bView ? EScriptMemberKind.WritableProperty : EScriptMemberKind.ReadOnlyProperty;
            if (!ScriptPropertyClassifier.RuleFor(Symbol, Kind, bOwnerIsNativeObject: true).bStored)
            {
                return null;
            }

            string Where = $"{FilePath}({Line(Property)}): '{Symbol.Name}'";
            if (Symbol.IsAbstract)
            {
                Errors.Add($"{Where} is abstract, and a stored property needs storage of its own. Declare it on the class that holds the value.");
                return null;
            }
            if (Property.AccessorList == null
                || Property.AccessorList.Accessors.Any(Accessor => Accessor.Body != null || Accessor.ExpressionBody != null))
            {
                Errors.Add($"{Where} has accessor bodies, but the engine keeps a stored property's value in native memory, so a body would "
                         + "read and write a copy the inspector, saves and prefabs never see. Declare it as '{ get; set; }' or as a field.");
                return null;
            }
            if (Property.AccessorList.Accessors.Any(Accessor => Accessor.IsKind(SyntaxKind.InitAccessorDeclaration)))
            {
                Errors.Add($"{Where} has an init accessor, but the engine assigns a stored value after construction. "
                         + "Use 'private set' to keep it read-only to callers.");
                return null;
            }

            AccessorDeclarationSyntax? Setter = Property.AccessorList.Accessors
                .FirstOrDefault(Accessor => Accessor.IsKind(SyntaxKind.SetAccessorDeclaration));
            return new FStoredMember
            {
                Declaration = Property,
                At = Property,
                Symbol = Symbol,
                Type = Symbol.Type,
                Initializer = Property.Initializer,
                bReadOnly = Setter == null,
                SetKeyword = Setter != null && Setter.Modifiers.Count > 0 ? Setter.Modifiers.ToString() + " set" : "set",
            };
        }

        private SyntaxNode RewriteClass(ClassDeclarationSyntax Node, List<FStoredMember> Stored, List<FRpcMethod> Rpcs)
        {

            // typeof rather than a spelled name, so the runtime identity rule names it and a nested class resolves too.
            string TypeName = Model.GetDeclaredSymbol(Node) is { } Declared
                ? Declared.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat)
                : Node.Identifier.Text;

            // Keyed by declaration, since `float A, B;` declares several members and each expands to statics plus a property.
            var Replacements = new Dictionary<MemberDeclarationSyntax, List<MemberDeclarationSyntax>>();
            var Defaults = new List<string>();

            foreach (FStoredMember Member in Stored)
            {
                string Name = Member.Symbol.Name;

                // The shared classifier also runs in the IDE analyzer, so what fails here was already underlined there.
                FScriptPropertyClassification Classification = ScriptPropertyClassifier.Classify(Member.Type);
                if (!Classification.IsSupported)
                {
                    Errors.Add($"{FilePath}({Line(Member.At)}): [Property] '{Name}' has type "
                             + $"'{Member.Type.ToDisplayString()}': {Classification.Rejection}");
                    continue;
                }

                // A container is a view over native storage, so it is filled rather than assigned.
                if (Classification.IsView && Member.Initializer != null)
                {
                    Errors.Add($"{FilePath}({Line(Member.At)}): [Property] '{Name}' is a container. It is a "
                             + "view over the native storage, so it cannot be initialized; add to it in OnReady instead.");
                    continue;
                }

                // Rewriting it would hand DispatchAction a string instead of the live binding.
                if (Classification.KeepsManagedField)
                {
                    continue;
                }

                if (Member.Initializer != null)
                {
                    Defaults.Add($"{Name} = {Member.Initializer.Value};");
                }

                if (!Replacements.TryGetValue(Member.Declaration, out List<MemberDeclarationSyntax>? Built))
                {
                    Built = new List<MemberDeclarationSyntax>();
                    Replacements[Member.Declaration] = Built;
                }
                Built.AddRange(BuildProperty(Member, TypeName, Classification));
            }

            if (Errors.Count > 0)
            {
                return Node;
            }

            // The declared order is preserved: each field is replaced in place by its property, so the
            // inspector's row order still follows the source.
            var RpcBodies = new Dictionary<MethodDeclarationSyntax, MethodDeclarationSyntax>();
            foreach (FRpcMethod Rpc in Rpcs)
            {
                RpcBodies[Rpc.Syntax] = RouteRpc(Rpc);
            }

            var Members = new List<MemberDeclarationSyntax>();
            foreach (MemberDeclarationSyntax Member in Node.Members)
            {
                if (Replacements.TryGetValue(Member, out List<MemberDeclarationSyntax>? Built))
                {
                    Members.AddRange(Built);
                    continue;
                }
                if (Member is MethodDeclarationSyntax Method && RpcBodies.TryGetValue(Method, out MethodDeclarationSyntax? Routed))
                {
                    Members.Add(Routed);
                    continue;
                }
                Members.Add(Member);
            }

            if (Defaults.Count > 0)
            {
                Members.AddRange(BuildDefaultsMethod(Defaults));
            }

            return Node.WithMembers(SyntaxFactory.List(Members));
        }

        private int Line(SyntaxNode Node) => Node.GetLocation().GetLineSpan().StartLinePosition.Line + 1;

        private sealed class FRpcMethod
        {
            public required MethodDeclarationSyntax Syntax;
            public required IMethodSymbol Symbol;
        }

        private List<FRpcMethod> CollectRpcs(ClassDeclarationSyntax Node)
        {
            var Result = new List<FRpcMethod>();
            var SeenNames = new HashSet<string>();
            foreach (MethodDeclarationSyntax Method in Node.Members.OfType<MethodDeclarationSyntax>().Where(MayBeRpc))
            {
                if (Model.GetDeclaredSymbol(Method) is not IMethodSymbol Symbol)
                {
                    continue;
                }

                AttributeData? Attribute = Symbol.GetAttributes().FirstOrDefault(Data =>
                    Data.AttributeClass?.ContainingType?.Name == "Rpc"
                    && Data.AttributeClass.Name is "BroadcastAttribute" or "HostAttribute" or "OwnerAttribute");
                if (Attribute == null)
                {
                    continue;
                }

                string Where = $"{FilePath}({Line(Method)}): [Rpc] '{Symbol.Name}'";
                if (!Symbol.ReturnsVoid || Symbol.IsStatic || Symbol.IsGenericMethod || Symbol.IsAsync)
                {
                    Errors.Add($"{Where} must be a non-static, non-generic, non-async method returning void, since the call returns before it runs remotely.");
                    continue;
                }
                if (Symbol.Parameters.Any(Parameter => Parameter.RefKind != RefKind.None))
                {
                    Errors.Add($"{Where} has a ref or out parameter, which a remote call has no way to hand back.");
                    continue;
                }
                if (!DerivesFrom(Symbol.ContainingType, EntityScriptSymbol))
                {
                    Errors.Add($"{Where} is declared on {Symbol.ContainingType.Name}, and only an CEntityScript has an entity to route the call through.");
                    continue;
                }
                if (!SeenNames.Add(Symbol.Name))
                {
                    Errors.Add($"{Where} is overloaded, and a remote call is matched by name alone.");
                    continue;
                }
                if (Method.Body == null && Method.ExpressionBody == null)
                {
                    Errors.Add($"{Where} has no body to route.");
                    continue;
                }

                Result.Add(new FRpcMethod
                {
                    Syntax = Method,
                    Symbol = Symbol,
                });
            }
            return Result;
        }

        // Every declarator of one field shares its attributes, so the first one answers for all of them.
        private bool IsStored(FieldDeclarationSyntax Field, bool bNativeBacked)
        {
            VariableDeclaratorSyntax? First = Field.Declaration.Variables.FirstOrDefault();
            return First != null && Model.GetDeclaredSymbol(First) is IFieldSymbol Symbol
                && ScriptPropertyClassifier.RuleFor(Symbol, EScriptMemberKind.Field, bNativeBacked).bStored;
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

        private static string Qualify(ITypeSymbol Type) => Type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);

        // One line on the body's first line, so no later line number moves. Native serializes the frame as it does a C++ NET_RPC.
        private MethodDeclarationSyntax RouteRpc(FRpcMethod Rpc)
        {
            var Prefix = new StringBuilder();
            Prefix.Append("if (global::LuminaSharp.RpcRuntime.Begin(this, \"").Append(Rpc.Symbol.Name).Append("\", out var __rpc)) { ");
            foreach (IParameterSymbol Parameter in Rpc.Symbol.Parameters)
            {
                Prefix.Append("__rpc.Add<").Append(Qualify(Parameter.Type)).Append(">(").Append(Parameter.Name).Append("); ");
            }
            Prefix.Append("if (__rpc.Send()) { return; } } ");

            StatementSyntax Gate = SyntaxFactory.ParseStatement(Prefix.ToString());
            MethodDeclarationSyntax Method = Rpc.Syntax;

            if (Method.Body != null)
            {
                return Method.WithBody(Method.Body.WithStatements(Method.Body.Statements.Insert(0, Gate)));
            }

            // An expression body becomes a block on the same line, keeping the line count.
            ArrowExpressionClauseSyntax Arrow = Method.ExpressionBody!;
            BlockSyntax Block = SyntaxFactory.Block(Gate, SyntaxFactory.ExpressionStatement(Arrow.Expression))
                .WithLeadingTrivia(Arrow.GetLeadingTrivia())
                .WithTrailingTrivia(Method.SemicolonToken.TrailingTrivia);
            return Method.WithExpressionBody(null).WithSemicolonToken(default).WithBody(Block);
        }



        /// <summary>
        /// The property replacing one field. Wrapped in #line directives pointing back at the field, so a
        /// stack trace, a breakpoint and any later diagnostic in the file still name the line the author
        /// wrote rather than an offset into generated text.
        /// </summary>
        private IEnumerable<MemberDeclarationSyntax> BuildProperty(FStoredMember Member, string TypeName, FScriptPropertyClassification Classification)
        {
            string Name = Member.Symbol.Name;
            SyntaxNode Declarator = Member.At;
            string Type = Member.Type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
            string Offset = $"__lazyoff_{Name}.Get(typeof({TypeName}), \"{Name}\")";
            string Token = $"__lazyprop_{Name}.Get(typeof({TypeName}), \"{Name}\")";
            bool bWritable = !Classification.IsView && !Member.bReadOnly;

            string Get;
            string? Set = null;
            string Unbound = "default";

            switch (Classification.Access)
            {
                case EScriptAccess.Blittable:
                    Get = $"global::System.Runtime.CompilerServices.Unsafe.ReadUnaligned<{Type}>((void*)((nint)Handle + {Offset}))";
                    Set = $"global::System.Runtime.CompilerServices.Unsafe.WriteUnaligned((void*)((nint)Handle + {Offset}), value)";
                    break;

                // The minted enum property is its underlying type's width, so the enum is read in place.
                case EScriptAccess.Enum:
                    Get = $"global::System.Runtime.CompilerServices.Unsafe.ReadUnaligned<{Type}>((void*)((nint)Handle + {Offset}))";
                    Set = $"global::System.Runtime.CompilerServices.Unsafe.WriteUnaligned((void*)((nint)Handle + {Offset}), value)";
                    break;

                // A string member makes the native value a different shape from the managed one, so it crosses field by field.
                case EScriptAccess.MarshalledStruct:
                    Get = $"global::LuminaSharp.ScriptStructMarshal<{Type}>.Read((nint)Handle + {Offset})";
                    Set = $"global::LuminaSharp.ScriptStructMarshal<{Type}>.Write((nint)Handle + {Offset}, value)";
                    break;

                case EScriptAccess.String:
                    Get = $"global::LuminaSharp.NativeMarshal.ReadString((nint)Handle + {Offset})";
                    Set = $"global::LuminaSharp.Native.PropSetString(Handle, {Token}, value)";
                    Unbound = "\"\"";
                    break;

                // Every asset-reference type is stored natively as one FSoftObjectPath, so all of them go
                // through the path. Routed by the ISoftObjectReference interface rather than by type name: a new
                // asset-reference type implements it and needs nothing here.
                case EScriptAccess.AssetPath:
                    Get = $"global::LuminaSharp.SoftObjectReferenceMarshal.Read<{Type}>("
                        + $"global::LuminaSharp.Native.PropGetAssetPath(Handle, {Token}))";
                    Set = $"global::LuminaSharp.Native.PropSetAssetPath(Handle, {Token}, "
                        + $"global::LuminaSharp.SoftObjectReferenceMarshal.Write(value))";
                    break;

                // The canonical wrapper, so reading twice returns the same instance and reference equality
                // means what a script author expects.
                // Through NativeObjectMarshal, not Wrapper<T> and value.Handle directly: both are internal or
                // protected, so generated code in the USER's assembly cannot reach them.
                case EScriptAccess.Object:
                    Get = $"global::LuminaSharp.NativeObjectMarshal.FromHandle<{Type}>("
                        + $"global::LuminaSharp.Native.PropGetObject(Handle, {Token}))";
                    Set = $"global::LuminaSharp.Native.PropSetObject(Handle, {Token}, "
                        + "global::LuminaSharp.NativeObjectMarshal.ToHandle(value))";
                    break;

                // A hard object reference. Stored natively as an object property (the same as a C++
                // TStrongObjectPtr), so it keeps its target alive; the value is just the pointer.
                case EScriptAccess.ObjectPtr:
                    Get = $"new {Type}(global::LuminaSharp.Native.PropGetObject(Handle, {Token}))";
                    Set = $"global::LuminaSharp.Native.PropSetObject(Handle, {Token}, value.NativeHandle)";
                    break;

                case EScriptAccess.MapView:
                {
                    string View = $"global::Lumina.THashMap<{Qualified(Classification.Key!)}, "
                                + $"{Qualified(Classification.Value!)}>";
                    Type = View;
                    Get = $"new {View}((nint)Handle + {Offset}, (nint)global::LuminaSharp.Native.PropMapOps({Token}))";
                    break;
                }

                // One view for every element flavour -- plain value, FString, TStrongObjectPtr<T>. What differs
                // per element is how a slot is read and written, and TVector routes that through
                // ElementMarshal, so nothing here needs to know which it is.
                case EScriptAccess.ListView:
                {
                    string View = $"global::Lumina.TVector<{Qualified(Classification.Element!)}>";
                    Type = View;
                    Get = $"new {View}((nint)Handle + {Offset}, (nint)global::LuminaSharp.Native.PropVectorOps({Token}))";
                    break;
                }

                // Listed rather than folded into a default, so adding a kind to the shared classifier without
                // teaching this emitter fails loudly here instead of being silently emitted as a TVector.
                default:
                    Errors.Add($"{FilePath}({Line(Declarator)}): [Property] '{Name}' classified as "
                             + $"{Classification.Access}, which this emitter does not handle. This is an engine bug.");
                    return System.Array.Empty<MemberDeclarationSyntax>();
            }

            // Built first, so only the cache the accessors actually reached for gets emitted below.
            var Body = new StringBuilder();
            // The attributes come across verbatim, and they are not decoration: TypeLibrary discovers a
            // member BY [Property] at run time, and reads Category/Tooltip/Min/Max off it to build the
            // inspector row. Dropping them here would compile perfectly and publish nothing.
            foreach (AttributeListSyntax List in Member.Declaration.AttributeLists)
            {
                // A field-targeted attribute on an auto-property has no backing field left to land on.
                if (Member.Declaration is PropertyDeclarationSyntax && List.Target?.Identifier.IsKind(SyntaxKind.FieldKeyword) == true)
                {
                    continue;
                }
                Body.AppendLine(List.ToString());
            }
            Body.Append(Accessibility(Member.Declaration.Modifiers)).Append(' ').Append(Type).Append(' ').AppendLine(Name);
            Body.AppendLine("{");
            // Gated on HasNativeStorage: the schema pass creates one UNBOUND instance per script type purely
            // to describe it, and reading through a null handle there is an access violation on load.
            Body.Append("    get => HasNativeStorage ? ").Append(Get).Append(" : ").Append(Unbound).AppendLine(";");
            bool bSync = HasAttributeNamed(Member.Declaration.AttributeLists, "Sync");
            if (bSync && Classification.IsView)
            {
                Errors.Add($"{FilePath}({Line(Declarator)}): [Sync] '{Name}' is a container, and only whole values replicate. "
                         + "Sync a count or an id instead, or send the contents through an RPC.");
                return System.Array.Empty<MemberDeclarationSyntax>();
            }
            if (bWritable && Set != null && bSync)
            {
                if (!BuildSyncSetter(Body, Member, Type, Get, Set))
                {
                    return System.Array.Empty<MemberDeclarationSyntax>();
                }
            }
            else if (bWritable && Set != null)
            {
                Body.Append("    ").Append(Member.SetKeyword).Append(" { if (HasNativeStorage) { ").Append(Set).AppendLine("; } }");
            }
            Body.AppendLine("}");

            string BodyText = Body.ToString();

            var Builder = new StringBuilder();
            if (BodyText.Contains("__lazyoff_" + Name))
            {
                Builder.Append("private static global::LuminaSharp.LazyPropertyOffset __lazyoff_").Append(Name).AppendLine(";");
            }
            if (BodyText.Contains("__lazyprop_" + Name))
            {
                Builder.Append("private static global::LuminaSharp.LazyPropertyToken __lazyprop_").Append(Name).AppendLine(";");
            }
            Builder.Append(BodyText);
            // Parsed as a class body rather than with ParseMemberDeclaration, which returns only the FIRST
            // member it finds -- that silently dropped the property and kept just the offset static.
            List<MemberDeclarationSyntax> Members = ParseMembers(Builder.ToString()).ToList();
            if (Members.Count == 0)
            {
                return Members;
            }

            // #line, attached as trivia rather than written into the parsed text: inside a wrapper class the
            // directive binds to the class's own brace token, so extracting the members drops it silently.
            //
            // The first maps the expansion back to the field the author wrote; the second hands the counter
            // to the line just past it, so every LATER line in the file keeps its real number instead of
            // drifting by however many lines this expanded to. Without them a stack trace from script code
            // names the wrong statement.
            const int OneBased = 1;
            const int NextLine = 1;
            Members[0] = Members[0].WithLeadingTrivia(Directive(Line(Declarator)));
            Members[^1] = Members[^1].WithTrailingTrivia(
                Members[^1].GetTrailingTrivia().AddRange(
                    Directive(Member.Declaration.GetLocation().GetLineSpan().EndLinePosition.Line + OneBased + NextLine)));
            return Members;
        }

        private static bool IsNumber(ITypeSymbol Type) => Type.SpecialType is SpecialType.System_Single or SpecialType.System_Double
            or SpecialType.System_Byte or SpecialType.System_SByte or SpecialType.System_Int16 or SpecialType.System_UInt16
            or SpecialType.System_Int32 or SpecialType.System_UInt32 or SpecialType.System_Int64 or SpecialType.System_UInt64;

        // Snaps, writes, then on a real change marks dirty, forwards an owner's write to the host and runs the change handler.
        private bool BuildSyncSetter(StringBuilder Body, FStoredMember Member, string Type, string Get, string Set)
        {
            ISymbol Symbol = Member.Symbol;
            string Where = $"{FilePath}({Line(Member.At)}): [Sync] '{Symbol.Name}'";
            AttributeData? Sync = Symbol.GetAttributes().FirstOrDefault(Data => Data.AttributeClass?.Name == "SyncAttribute");
            AttributeData? Change = Symbol.GetAttributes().FirstOrDefault(Data => Data.AttributeClass?.Name == "ChangeAttribute");

            uint Flags = Sync != null && Sync.ConstructorArguments.Length > 0 && Sync.ConstructorArguments[0].Value is uint Raw ? Raw : 0u;
            bool bFromOwner = (Flags & 1u) != 0;
            double Quantize = 0.0;
            string? Validate = null;
            if (Sync != null)
            {
                foreach (KeyValuePair<string, TypedConstant> Named in Sync.NamedArguments)
                {
                    if (Named.Key == "Quantize" && Named.Value.Value is double Snap)
                    {
                        Quantize = Snap;
                    }
                    else if (Named.Key == "Validate" && Named.Value.Value is string Method)
                    {
                        Validate = Method;
                    }
                }
            }

            if (Quantize > 0.0 && !IsNumber(Member.Type))
            {
                Errors.Add($"{Where} sets Quantize, which only a number can snap to.");
                return false;
            }
            if (bFromOwner && !DerivesFrom(Symbol.ContainingType, EntityScriptSymbol))
            {
                Errors.Add($"{Where} is FromOwner, and only an CEntityScript has an owner to take the write from.");
                return false;
            }
            if (Validate != null && !bFromOwner)
            {
                Errors.Add($"{Where} names Validate without SyncFlags.FromOwner, and only an owner's write is validated.");
                return false;
            }
            if (Validate != null && !HasValidator(Symbol.ContainingType, Validate, Member.Type))
            {
                Errors.Add($"{Where} names Validate method '{Validate}', which must take one {Member.Type.ToDisplayString()} and return bool.");
                return false;
            }

            string Step = Quantize.ToString("R", System.Globalization.CultureInfo.InvariantCulture) + "d";
            Body.Append("    ").Append(Member.SetKeyword).Append(" { if (HasNativeStorage) { ");
            if (Quantize > 0.0)
            {
                Body.Append("value = (").Append(Type).Append(")(global::System.Math.Round((double)value / ").Append(Step).Append(") * ").Append(Step).Append("); ");
            }
            Body.Append("var __old = ").Append(Get).Append("; ").Append(Set)
                .Append("; if (!global::System.Collections.Generic.EqualityComparer<").Append(Type)
                .Append(">.Default.Equals(__old, value)) { global::LuminaSharp.SyncRuntime.MarkDirty(Handle); ");

            // Native sends the stored value through the field's FProperty, as it does for a C++ PROPERTY(Sync = FromOwner).
            if (bFromOwner)
            {
                Body.Append("if (global::LuminaSharp.SyncRuntime.ShouldForward(this)) { global::LuminaSharp.SyncRuntime.Forward(this, \"")
                    .Append(Symbol.Name).Append("\"); } ");
            }

            if (Change != null)
            {
                Body.Append("global::LuminaSharp.SyncRuntime.Changed(this, \"").Append(Symbol.Name).Append("\", __old, value); ");
            }

            Body.AppendLine("} } }");
            return true;
        }

        private static bool HasValidator(INamedTypeSymbol Type, string Name, ITypeSymbol ValueType)
        {
            for (INamedTypeSymbol? Current = Type; Current != null; Current = Current.BaseType)
            {
                foreach (IMethodSymbol Method in Current.GetMembers(Name).OfType<IMethodSymbol>())
                {
                    if (Method.Parameters.Length == 1 && Method.ReturnType.SpecialType == SpecialType.System_Boolean
                        && SymbolEqualityComparer.Default.Equals(Method.Parameters[0].Type, ValueType))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        private SyntaxTriviaList Directive(int SourceLine)
        {
            return SyntaxFactory.ParseLeadingTrivia($"#line {SourceLine} \"{FilePath.Replace("\\", "\\\\")}\"\n");
        }

        private static IEnumerable<MemberDeclarationSyntax> ParseMembers(string Text)
        {
            // The newlines matter: a #line directive is only recognised at the START of a line, so splicing
            // the wrapper onto the same line silently demotes it to nothing and the mapping is lost.
            CompilationUnitSyntax Unit = SyntaxFactory.ParseCompilationUnit("class __Members__ {\n" + Text + "\n}");
            return Unit.Members.OfType<ClassDeclarationSyntax>().SelectMany(Class => Class.Members);
        }

        /// <summary>The class's declared initializers, replayed against the class default object. Not a
        /// constructor: the managed wrapper is created AFTER the native object is loaded, so assigning there
        /// would overwrite every authored value with the declared default.</summary>
        private static IEnumerable<MemberDeclarationSyntax> BuildDefaultsMethod(List<string> Defaults)
        {
            var Builder = new StringBuilder();
            // The base virtual is plain protected (NativeObject.ApplyScriptDefaults wraps it for the engine).
            Builder.AppendLine("protected override void __ApplyScriptDefaults()");
            Builder.AppendLine("{");
            Builder.AppendLine("    base.__ApplyScriptDefaults();");
            foreach (string Assignment in Defaults)
            {
                Builder.Append("    ").AppendLine(Assignment);
            }
            Builder.AppendLine("}");
            return ParseMembers(Builder.ToString());
        }

        private static string Accessibility(SyntaxTokenList DeclaredModifiers)
        {
            // 'unsafe' because every accessor dereferences the container pointer; the field's own modifiers
            // (public/internal) carry over so the property is reachable exactly where the field was.
            List<string> Modifiers = DeclaredModifiers
                .Select(Token => Token.Text)
                .Where(Text => Text is "public" or "private" or "protected" or "internal" or "new" or "virtual" or "override" or "sealed")
                .ToList();
            if (!Modifiers.Any(Text => Text is "public" or "private" or "protected" or "internal"))
            {
                Modifiers.Add("private");
            }
            Modifiers.Add("unsafe");
            return string.Join(" ", Modifiers);
        }

        /// <summary>A type argument as the emitted code must spell it: fully qualified, so a view's element
        /// type resolves the same wherever the user's own usings happen to point.</summary>
        private static string Qualified(ITypeSymbol Type) =>
            Type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
    }
}
