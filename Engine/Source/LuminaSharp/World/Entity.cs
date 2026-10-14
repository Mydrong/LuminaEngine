using System;

namespace LuminaSharp;

public readonly struct FEntity : IEquatable<FEntity>
{
    public readonly uint Id;

    public FEntity(uint Id)
    {
        this.Id = Id;
    }

    public static readonly FEntity Null = new(0xFFFFFFFFu);

    public bool IsNull => Id == Null.Id;

    public bool Equals(FEntity Other) => Id == Other.Id;

    public override bool Equals(object? Obj) => Obj is FEntity Other && Equals(Other);

    public override int GetHashCode() => (int)Id;

    public static bool operator ==(FEntity Left, FEntity Right) => Left.Id == Right.Id;

    public static bool operator !=(FEntity Left, FEntity Right) => Left.Id != Right.Id;

    public override string ToString() => IsNull ? "FEntity(null)" : $"FEntity({Id})";
}
