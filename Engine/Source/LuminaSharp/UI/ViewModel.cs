using System;
using JetBrains.Annotations;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace LuminaSharp;

public abstract class ViewModel
{
    /// <summary>The live binding, or null while the model is not registered. Set by <see cref="UIDataModel"/>.</summary>
    internal UIDataModel? Binding;

    /// <summary>True once this view-model is bound to a document and pushing changes to the view.</summary>
    public bool IsBound => Binding != null && Binding.IsValid;

    /// <summary>
    /// Setter helper for a bound property: assigns the backing field and, if the value actually changed,
    /// pushes it to the view. Use it in property setters: <c>set =&gt; Set(ref _field, value);</c>.
    /// </summary>
    protected void Set<T>(ref T Field, T Value, [CallerMemberName] string Name = "")
    {
        if (EqualityComparer<T>.Default.Equals(Field, Value))
        {
            return;
        }
        Field = Value;
        Binding?.OnPropertyChanged(Name);
    }

    /// <summary>
    /// Force a refresh of one bound property by name (for computed values, or fields mutated without
    /// <see cref="Set{T}"/>). Defaults to the calling property.
    /// </summary>
    protected void NotifyChanged([CallerMemberName] string Name = "")
    {
        Binding?.OnPropertyChanged(Name);
    }

    /// <summary>Re-push every bound property to the view. Cheap for a handful of variables; prefer
    /// <see cref="Set{T}"/> (push-on-change) over calling this every frame to avoid needless work.</summary>
    public void Refresh()
    {
        Binding?.PushAll();
    }
}
