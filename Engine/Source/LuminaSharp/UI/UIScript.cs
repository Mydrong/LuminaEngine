using System;
using JetBrains.Annotations;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using LuminaSharp;

namespace LuminaSharp
{
    // Fills a UIElement field or property with the element of this id, or of the member's own name, each time the document loads.
    [MeansImplicitUse(ImplicitUseKindFlags.Access | ImplicitUseKindFlags.Assign)]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class ElementAttribute : Attribute
    {
        public ElementAttribute(string? Id = null)
        {
            this.Id = Id;
        }

        public string? Id { get; }
    }

    // Which members [Element] fills on each script class, found once per class.
    internal static class UIScriptLayout
    {
        internal sealed class FElementSlot
        {
            public required string Id;
            public required string MemberName;
            public required Action<object, object?> Set;
        }

        // Weak, so a hot reload's old script types are not pinned.
        private static readonly ConditionalWeakTable<Type, FElementSlot[]> Elements = new();

        public static FElementSlot[] ElementSlots(Type Type) => Elements.GetValue(Type, Find);

        private static FElementSlot[] Find(Type Type)
        {
            var Slots = new List<FElementSlot>();
            const BindingFlags Flags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly;
            for (Type? Current = Type; Current != null && Current != typeof(Lumina.CUIScript); Current = Current.BaseType)
            {
                foreach (FieldInfo Field in Current.GetFields(Flags))
                {
                    if (Field.GetCustomAttribute<ElementAttribute>() is { } Attribute && Field.FieldType == typeof(UIElement))
                    {
                        Slots.Add(new FElementSlot { Id = Attribute.Id ?? Field.Name, MemberName = Field.Name, Set = Field.SetValue });
                    }
                }
                foreach (PropertyInfo Property in Current.GetProperties(Flags))
                {
                    if (Property.GetCustomAttribute<ElementAttribute>() is { } Attribute && Property.PropertyType == typeof(UIElement)
                        && PropertyAccessor.Setter(Property) is { } Setter)
                    {
                        Slots.Add(new FElementSlot { Id = Attribute.Id ?? Property.Name, MemberName = Property.Name, Set = Setter });
                    }
                }
            }
            return Slots.ToArray();
        }
    }
}

namespace Lumina
{
    // The C# side of an entity script that is its own screen UI, the same CUIScript a C++ UI script derives.
    [DispatchWhen(nameof(OnDocumentLoaded), nameof(FillsElements))]
    public partial class CUIScript
    {
        // The loaded document, invalid while the script has none.
        public UIDocument View => new((ulong)NativeObjectMarshal.ToHandle(World), CUILibrary.GetScriptDocument(this));

        // The name the document's data-model attribute uses, from [Reflect(DataModel)] or the class name.
        public string ModelName => ReflectAttribute.DataModelOf(GetType());

        // The class default object takes [Reflect(ShowOnStart, Interactive)], where a C++ subclass would set them in its constructor.
        protected override void __ApplyScriptDefaults()
        {
            base.__ApplyScriptDefaults();
            if (ReflectAttribute.Nearest(GetType(), Reflect => Reflect.HasShowOnStart) is { } ShowOnStart)
            {
                bShowOnStart = ShowOnStart.ShowOnStart;
            }
            if (ReflectAttribute.Nearest(GetType(), Reflect => Reflect.HasInteractive) is { } Interactive)
            {
                bInteractive = Interactive.Interactive;
            }
        }

        // Points every [Element] member at the newly loaded document before the script's own OnDocumentLoaded runs.
        partial void __BeforeScriptEvent_OnDocumentLoaded()
        {
            UIDocument Loaded = View;
            foreach (UIScriptLayout.FElementSlot Slot in UIScriptLayout.ElementSlots(GetType()))
            {
                UIElement Element = Loaded.GetElementById(Slot.Id);
                if (!Element.IsValid)
                {
                    Debug.LogWarning($"[UI] {GetType().Name}.{Slot.MemberName} found no element with id '{Slot.Id}'.");
                }
                Slot.Set(this, Element);
            }
        }

        private static bool FillsElements(TypeLibrary Library, Type ScriptType) => UIScriptLayout.ElementSlots(ScriptType).Length > 0;
    }
}
