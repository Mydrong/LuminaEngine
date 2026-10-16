using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace LuminaSharp;

// What a type exposes to RML through Bind, found once per type and shared by every binding of it and by the editor's preview.
internal sealed class UIModelShape
{
    internal sealed class FScalar
    {
        public required string Name;
        public required string MemberName;
        public required Type ValueType;
        public required Lumina.EUIVarType VarType;

        // Exactly one is set, compiled to the value the view stores so a frame's poll neither reflects nor boxes.
        public Func<object, double>? ReadNumber;
        public Func<object, string?>? ReadText;
        public Action<object, object?>? Set;
    }

    internal sealed class FListItem
    {
        public required string Name;
        public required Func<object, object?> Get;
    }

    internal sealed class FList
    {
        public required string Name;
        public required string MemberName;
        public required Type ItemType;
        public required Func<object, object?> Get;
        public required Func<object?, long> Signature;
        public required FListItem[] Items;
    }

    internal sealed class FCommand
    {
        public required string Name;
        public required MethodInfo Method;
        public required ParameterInfo[] Parameters;
    }

    public readonly List<FScalar> Scalars = new();
    public readonly List<FList> Lists = new();
    public readonly List<FCommand> Commands = new();

    // Weak, so a hot reload's old script types are not pinned by the cache.
    private static readonly ConditionalWeakTable<Type, UIModelShape> Cache = new();

    private const BindingFlags Declared = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly;

    public static UIModelShape Of(Type Type) => Cache.GetValue(Type, Build);

    public bool IsEmpty => Scalars.Count == 0 && Lists.Count == 0 && Commands.Count == 0;

    // The engine's own base classes carry no Bind members, so the walk stops where the user's types end.
    private static IEnumerable<Type> UserHierarchy(Type Type)
    {
        Assembly Engine = typeof(UIModelShape).Assembly;
        for (Type? Current = Type; Current != null && Current != typeof(object) && Current.Assembly != Engine; Current = Current.BaseType)
        {
            yield return Current;
        }
    }

    private static UIModelShape Build(Type Type)
    {
        var Shape = new UIModelShape();
        var Taken = new HashSet<string>(StringComparer.Ordinal);

        foreach (Type Level in UserHierarchy(Type))
        {
            foreach (PropertyInfo Property in Level.GetProperties(Declared))
            {
                if (BoundName(Property) is not { } Name || Property.GetMethod == null)
                {
                    continue;
                }
                Shape.AddValue(Type, Property, Name, Property.PropertyType,
                    PropertyAccessor.Getter(Property), Property.SetMethod != null ? PropertyAccessor.Setter(Property) : null, Taken);
            }

            foreach (FieldInfo Field in Level.GetFields(Declared))
            {
                if (BoundName(Field) is not { } Name || Field.IsStatic)
                {
                    continue;
                }
                Action<object, object?>? Setter = Field.IsInitOnly ? null : Field.SetValue;
                Shape.AddValue(Type, Field, Name, Field.FieldType, Field.GetValue, Setter, Taken);
            }

            foreach (MethodInfo Method in Level.GetMethods(Declared))
            {
                FunctionAttribute? Function = Method.GetCustomAttribute<FunctionAttribute>();
                if (Function == null || !Function.Has(ESpecifier.Bind) || Method.IsStatic || Method.IsGenericMethodDefinition)
                {
                    continue;
                }
                string CommandName = Function.Bind ?? Method.Name;
                if (!Taken.Add(CommandName))
                {
                    Debug.LogWarning($"[UI] {Type.Name}.{Method.Name}: the name '{CommandName}' is bound twice; this one is skipped.");
                    continue;
                }
                Shape.Commands.Add(new FCommand { Name = CommandName, Method = Method, Parameters = Method.GetParameters() });
            }
        }
        return Shape;
    }

    private void AddValue(Type Owner, MemberInfo Member, string Name, Type ValueType, Func<object, object?> Get,
        Action<object, object?>? Set, HashSet<string> Taken)
    {
        string MemberName = Member.Name;
        if (!Taken.Add(Name))
        {
            Debug.LogWarning($"[UI] {Owner.Name}.{MemberName}: the name '{Name}' is bound twice; this one is skipped.");
            return;
        }

        if (TryMapType(ValueType, out Lumina.EUIVarType VarType))
        {
            var Scalar = new FScalar { Name = Name, MemberName = MemberName, ValueType = ValueType, VarType = VarType, Set = Set };
            if (VarType == Lumina.EUIVarType.String)
            {
                Scalar.ReadText = CompileReader<string?>(Member, Value => Value);
            }
            else
            {
                Scalar.ReadNumber = CompileReader<double>(Member, Value => ToDouble(Value, ValueType));
            }
            Scalars.Add(Scalar);
            return;
        }

        if (TryGetItemType(ValueType, out Type ItemType) && BuildItems(ItemType) is { Length: > 0 } Items)
        {
            Lists.Add(new FList { Name = Name, MemberName = MemberName, ItemType = ItemType, Get = Get, Items = Items,
                Signature = typeof(UIModelShape).GetMethod(nameof(SignatureOf), BindingFlags.NonPublic | BindingFlags.Static)!
                    .MakeGenericMethod(ItemType).CreateDelegate<Func<object?, long>>() });
            return;
        }

        Debug.LogWarning($"[UI] {Owner.Name}.{MemberName}: [Property(Bind)] type '{ValueType.Name}' is not bindable. Use a number, bool, enum, "
            + "string, or a collection of a type with [Property(Bind)] members.");
    }

    // Target => Convert((Owner)Target.Member), compiled against the member's declaring type so a private base member still reads.
    private static Func<object, TResult> CompileReader<TResult>(MemberInfo Member, Func<Expression, Expression> Convert)
    {
        ParameterExpression Target = Expression.Parameter(typeof(object), "Target");
        Expression Owner = Expression.Convert(Target, Member.DeclaringType!);
        Expression Value = Member is PropertyInfo Property ? Expression.Property(Owner, Property) : Expression.Field(Owner, (FieldInfo)Member);
        return Expression.Lambda<Func<object, TResult>>(Convert(Value), Target).Compile();
    }

    private static Expression ToDouble(Expression Value, Type ValueType)
    {
        if (ValueType == typeof(bool))
        {
            return Expression.Condition(Value, Expression.Constant(1.0), Expression.Constant(0.0));
        }
        if (ValueType.IsEnum)
        {
            Value = Expression.Convert(Value, Enum.GetUnderlyingType(ValueType));
        }
        return Expression.Convert(Value, typeof(double));
    }

    private static string? BoundName(MemberInfo Member)
    {
        PropertyAttribute? Property = Member.GetCustomAttribute<PropertyAttribute>();
        return Property != null && Property.Has(ESpecifier.Bind) ? Property.Bind ?? Member.Name : null;
    }

    private static FListItem[] BuildItems(Type ItemType)
    {
        var Items = new List<FListItem>();
        foreach (Type Level in UserHierarchy(ItemType))
        {
            foreach (PropertyInfo Property in Level.GetProperties(Declared))
            {
                if (BoundName(Property) is { } Name && Property.GetMethod != null)
                {
                    Items.Add(new FListItem { Name = Name, Get = PropertyAccessor.Getter(Property) });
                }
            }
            foreach (FieldInfo Field in Level.GetFields(Declared))
            {
                if (BoundName(Field) is { } Name && !Field.IsStatic)
                {
                    Items.Add(new FListItem { Name = Name, Get = Field.GetValue });
                }
            }
        }
        return Items.ToArray();
    }

    internal static bool TryMapType(Type Type, out Lumina.EUIVarType VarType)
    {
        if (Type.IsEnum) { VarType = Lumina.EUIVarType.Int; return true; }
        if (Type == typeof(bool)) { VarType = Lumina.EUIVarType.Bool; return true; }
        if (Type == typeof(string)) { VarType = Lumina.EUIVarType.String; return true; }
        if (Type == typeof(float)) { VarType = Lumina.EUIVarType.Float; return true; }
        if (Type == typeof(double)) { VarType = Lumina.EUIVarType.Double; return true; }
        if (Type == typeof(int) || Type == typeof(short) || Type == typeof(sbyte) || Type == typeof(byte)
            || Type == typeof(uint) || Type == typeof(ushort) || Type == typeof(long) || Type == typeof(ulong))
        {
            VarType = Lumina.EUIVarType.Int;
            return true;
        }
        VarType = default;
        return false;
    }

    private static bool TryGetItemType(Type Type, out Type ItemType)
    {
        ItemType = typeof(object);
        if (Type == typeof(string))
        {
            return false;
        }
        if (Type.IsGenericType && Type.GetGenericTypeDefinition() == typeof(IEnumerable<>))
        {
            ItemType = Type.GetGenericArguments()[0];
            return true;
        }
        foreach (Type Interface in Type.GetInterfaces())
        {
            if (Interface.IsGenericType && Interface.GetGenericTypeDefinition() == typeof(IEnumerable<>))
            {
                ItemType = Interface.GetGenericArguments()[0];
                return true;
            }
        }
        return false;
    }

    // Changes when the collection is replaced, resized or reordered. An item edited in place needs NotifyChanged.
    private static long SignatureOf<T>(object? Collection)
    {
        if (Collection is not IEnumerable<T> Items)
        {
            return 0;
        }

        long Hash = RuntimeHelpers.GetHashCode(Collection);
        if (Collection is List<T> List)
        {
            foreach (T Item in CollectionsMarshal.AsSpan(List))
            {
                Hash = Hash * 31 + ItemHash(Item);
            }
            return Hash;
        }
        if (Collection is T[] Array)
        {
            foreach (T Item in Array)
            {
                Hash = Hash * 31 + ItemHash(Item);
            }
            return Hash;
        }
        foreach (T Item in Items)
        {
            Hash = Hash * 31 + ItemHash(Item);
        }
        return Hash;
    }

    // A value item hashes by value, since the same struct read twice is never the same reference.
    private static int ItemHash<T>(T Item)
    {
        if (Item is null)
        {
            return 7;
        }
        return typeof(T).IsValueType || Item.GetType().IsValueType ? EqualityComparer<T>.Default.GetHashCode(Item) : RuntimeHelpers.GetHashCode(Item);
    }
}
