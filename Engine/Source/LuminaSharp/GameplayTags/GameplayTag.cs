using System;
using Lumina;

namespace LuminaSharp;

/// <summary>
/// A hierarchical gameplay tag (-style), e.g. <c>"Ability.Fire.Fireball"</c>. A lightweight value
/// wrapping an interned id from the process-global registry, so equality and hierarchical matching are
/// O(depth) integer compares. Request one with <see cref="Request"/>; the registry interns the tag and all
/// its ancestors. Used as message-bus channels (<see cref="Lumina.CWorld"/> Subscribe and Broadcast), entity/ability tags, etc.
/// </summary>
public readonly partial struct GameplayTag : IEquatable<GameplayTag>
{
    /// <summary>The interned registry id; 0 is <see cref="None"/>.</summary>
    public readonly uint Id;

    internal GameplayTag(uint Id)
    {
        this.Id = Id;
    }

    /// <summary>The invalid / empty tag.</summary>
    public static readonly GameplayTag None = default;

    /// <summary>Intern (or look up) a dotted tag name and its ancestors; returns <see cref="None"/> if empty.</summary>
    public static GameplayTag Request(string Name)
    {
        return string.IsNullOrEmpty(Name) ? None : new GameplayTag(CGameplayTagLibrary.RequestTag(Name));
    }

    public bool IsValid => Id != 0;

    /// <summary>The dotted tag name, or empty for <see cref="None"/>.</summary>
    public string Name => Id == 0 ? string.Empty : CGameplayTagLibrary.GetTagName(Id);

    /// <summary>The immediate parent tag (<c>"Ability.Fire.Fireball"</c> -> <c>"Ability.Fire"</c>), or None at the root.</summary>
    public GameplayTag Parent => new GameplayTag(CGameplayTagLibrary.GetParent(Id));

    /// <summary>
    /// Hierarchical match: true when this tag IS <paramref name="Other"/> or a descendant of it (this is at
    /// or below Other in the tree). So <c>Request("Ability.Fire.Fireball").Matches(Request("Ability.Fire"))</c>
    /// is true, but the reverse is false.
    /// </summary>
    public bool Matches(GameplayTag Other) => CGameplayTagLibrary.Matches(Id, Other.Id);

    /// <summary>Exact (non-hierarchical) tag equality.</summary>
    public bool MatchesExact(GameplayTag Other) => Id != 0 && Id == Other.Id;

    public bool Equals(GameplayTag Other) => Id == Other.Id;
    public override bool Equals(object? Obj) => Obj is GameplayTag Tag && Tag.Id == Id;
    public override int GetHashCode() => (int)Id;
    public static bool operator ==(GameplayTag A, GameplayTag B) => A.Id == B.Id;
    public static bool operator !=(GameplayTag A, GameplayTag B) => A.Id != B.Id;
    public override string ToString() => Id == 0 ? "GameplayTag.None" : Name;
}
