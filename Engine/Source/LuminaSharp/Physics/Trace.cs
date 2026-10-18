using System;
using Lumina;

namespace LuminaSharp;

/// Fluent physics query, s&amp;box style, as Trace.Ray(a, b).Ignore(self).Run(). A mutable builder over the World physics queries that allocates nothing until Run or RunAll, against the ambient Engine.World.
public struct Trace
{
    private CWorld World;
    private FVector3 From;
    private FVector3 To;
    private SQueryShape Shape;
    private bool bSwept;
    private FEntity IgnoreEntity;
    private ECollisionProfiles Mask;

    private static Trace Begin(FVector3 From, FVector3 To)
    {
        Trace T = default;
        T.World = Engine.World;
        T.From = From;
        T.To = To;
        T.Shape.Rotation = FQuat.Identity;
        T.IgnoreEntity = FEntity.Null;
        T.Mask = ECollisionProfiles.All;
        return T;
    }

    private static Trace Sweep(FVector3 From, FVector3 To, EQueryShapeType Type)
    {
        Trace T = Begin(From, To);
        T.Shape.Type = Type;
        T.bSwept = true;
        return T;
    }

    /// A ray between two world points.
    public static Trace Ray(FVector3 From, FVector3 To) => Begin(From, To);

    /// A ray from an origin along a direction for a distance.
    public static Trace Ray(FVector3 Origin, FVector3 Direction, float Distance)
        => Begin(Origin, Origin + Direction.Normalized() * Distance);

    /// A swept sphere, so a thick ray, between two points.
    public static Trace Sphere(float Radius, FVector3 From, FVector3 To)
    {
        Trace T = Sweep(From, To, EQueryShapeType.Sphere);
        T.Shape.Radius = Radius;
        return T;
    }

    /// A swept box, turned by Rotated.
    public static Trace Box(FVector3 HalfExtents, FVector3 From, FVector3 To)
    {
        Trace T = Sweep(From, To, EQueryShapeType.Box);
        T.Shape.HalfExtents = HalfExtents;
        return T;
    }

    /// A swept capsule standing along the up axis, the shape of a character, turned by Rotated.
    public static Trace Capsule(float Radius, float HalfHeight, FVector3 From, FVector3 To)
    {
        Trace T = Sweep(From, To, EQueryShapeType.Capsule);
        T.Shape.Radius = Radius;
        T.Shape.HalfHeight = HalfHeight;
        return T;
    }

    /// Turns a swept box or capsule.
    public Trace Rotated(FQuat Rotation)
    {
        Shape.Rotation = Rotation;
        return this;
    }

    /// Skip one entity's body, typically the caster's.
    public Trace Ignore(FEntity Entity)
    {
        IgnoreEntity = Entity;
        return this;
    }

    /// Skip the entity whose callback is running.
    public Trace IgnoreSelf()
    {
        IgnoreEntity = Engine.CurrentEntity;
        return this;
    }

    /// Only hit bodies whose collision layer intersects Mask.
    public Trace WithMask(ECollisionProfiles Mask)
    {
        this.Mask = Mask;
        return this;
    }

    /// Runs the query and returns the closest hit. Check bHit on the result.
    public SRayResult Run()
    {
        return bSwept
            ? World.ShapeCast(Shape, From, To, IgnoreEntity, Mask)
            : World.Raycast(From, To, IgnoreEntity, Mask);
    }

    /// Runs the query and returns every hit, near to far.
    public SRayResult[] RunAll()
    {
        return bSwept
            ? World.ShapeCastAll(Shape, From, To, IgnoreEntity, Mask)
            : World.RaycastAll(From, To, IgnoreEntity, Mask);
    }
}
