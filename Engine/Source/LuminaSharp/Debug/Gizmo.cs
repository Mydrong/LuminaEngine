using Lumina;

namespace LuminaSharp;

/// <summary>
/// Immediate-mode debug drawing for the current world (Dev/Debug only). State-driven like s&amp;box's Gizmo:
/// set <see cref="Color"/> / <see cref="Thickness"/> / <see cref="Duration"/> once, then draw. Forwards to CDebugDrawLibrary. Duration &lt;= 0 draws for one frame.
/// </summary>
public static class Gizmo
{
    public static Color Color = LuminaSharp.Color.White;
    public static float Thickness = 1.0f;
    public static float Duration = 0.0f;
    public static bool DepthTest = true;
    public static EDebugFillMode FillMode = EDebugFillMode.Translucent;

    public static void Line(FVector3 Start, FVector3 End)
        => CDebugDrawLibrary.DrawLine(Engine.World, Start, End, Color, Thickness, Duration, DepthTest);

    public static void Line(FVector3 Start, FVector3 End, Color Color)
        => CDebugDrawLibrary.DrawLine(Engine.World, Start, End, Color, Thickness, Duration, DepthTest);

    public static void Sphere(FVector3 Center, float Radius)
        => CDebugDrawLibrary.DrawSphere(Engine.World, Center, Radius, Color, Thickness, Duration, DepthTest);

    public static void Box(FVector3 Center, FVector3 HalfExtents)
        => CDebugDrawLibrary.DrawBox(Engine.World, Center, HalfExtents, FQuat.Identity, Color, Thickness, Duration, DepthTest);

    public static void Box(FVector3 Center, FVector3 HalfExtents, FQuat Rotation)
        => CDebugDrawLibrary.DrawBox(Engine.World, Center, HalfExtents, Rotation, Color, Thickness, Duration, DepthTest);

    public static void BoxCorners(FVector3 Center, FVector3 HalfExtents, FQuat Rotation, float CornerFraction = 0.25f)
        => CDebugDrawLibrary.DrawBoxCorners(Engine.World, Center, HalfExtents, Rotation, Color, CornerFraction, Thickness, Duration, DepthTest);

    public static void Capsule(FVector3 Start, FVector3 End, float Radius)
        => CDebugDrawLibrary.DrawCapsule(Engine.World, Start, End, Radius, Color, Thickness, Duration, DepthTest);

    public static void Cone(FVector3 Apex, FVector3 Direction, float AngleRadians, float Length)
        => CDebugDrawLibrary.DrawCone(Engine.World, Apex, Direction, AngleRadians, Length, Color, Thickness, Duration, DepthTest);

    public static void Arrow(FVector3 Start, FVector3 Direction, float Length, float HeadSize = 0.2f)
        => CDebugDrawLibrary.DrawArrow(Engine.World, Start, Direction, Length, Color, HeadSize, Thickness, Duration, DepthTest);

    public static void Circle(FVector3 Center, FVector3 Normal, float Radius, byte Segments = 32)
        => CDebugDrawLibrary.DrawCircle(Engine.World, Center, Normal, Radius, Color, Segments, Thickness, Duration, DepthTest);

    public static void Arc(FVector3 Center, FVector3 Normal, float Radius, float StartAngle, float SweepAngle, byte Segments = 16)
        => CDebugDrawLibrary.DrawArc(Engine.World, Center, Normal, Radius, StartAngle, SweepAngle, Color, Segments, Thickness, Duration, DepthTest);

    public static void Cylinder(FVector3 Start, FVector3 End, float Radius, byte Segments = 16)
        => CDebugDrawLibrary.DrawCylinder(Engine.World, Start, End, Radius, Color, Segments, Thickness, Duration, DepthTest);

    public static void Axes(FVector3 Origin, FQuat Rotation, float Length)
        => CDebugDrawLibrary.DrawAxes(Engine.World, Origin, Rotation, Length, Thickness, Duration, DepthTest);

    public static void Point(FVector3 Position, float Size)
        => CDebugDrawLibrary.DrawPoint(Engine.World, Position, Size, Color, Thickness, Duration, DepthTest);

    public static void SolidTriangles(System.ReadOnlySpan<SDebugDrawVertex> Vertices)
        => CDebugDrawLibrary.DrawSolidTriangles(Engine.World, Vertices, FillMode, Duration);

    public static void SolidTriangle(FVector3 A, FVector3 B, FVector3 C)
        => CDebugDrawLibrary.DrawSolidTriangle(Engine.World, A, B, C, Color, FillMode, Duration);

    public static void SolidQuad(FVector3 A, FVector3 B, FVector3 C, FVector3 D)
        => CDebugDrawLibrary.DrawSolidQuad(Engine.World, A, B, C, D, Color, FillMode, Duration);

    public static void SolidBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation)
        => CDebugDrawLibrary.DrawSolidBox(Engine.World, Center, HalfExtents, Rotation, Color, FillMode, Duration);

    public static void SolidSphere(FVector3 Center, float Radius)
        => CDebugDrawLibrary.DrawSolidSphere(Engine.World, Center, Radius, Color, FillMode, Duration);

    public static void SolidCylinder(FVector3 Start, FVector3 End, float Radius)
        => CDebugDrawLibrary.DrawSolidCylinder(Engine.World, Start, End, Radius, Color, FillMode, Duration);

    public static void SolidCone(FVector3 Apex, FVector3 Direction, float AngleRadians, float Length)
        => CDebugDrawLibrary.DrawSolidCone(Engine.World, Apex, Direction, AngleRadians, Length, Color, FillMode, Duration);

    public static void SolidCapsule(FVector3 Start, FVector3 End, float Radius)
        => CDebugDrawLibrary.DrawSolidCapsule(Engine.World, Start, End, Radius, Color, FillMode, Duration);

    public static void SolidCircle(FVector3 Center, FVector3 Normal, float Radius)
        => CDebugDrawLibrary.DrawSolidCircle(Engine.World, Center, Normal, Radius, Color, FillMode, Duration);

    public static void SolidArc(FVector3 Center, FVector3 Normal, float Radius, float StartAngle, float SweepAngle)
        => CDebugDrawLibrary.DrawSolidArc(Engine.World, Center, Normal, Radius, StartAngle, SweepAngle, Color, FillMode, Duration);

    public static void SolidArrow(FVector3 Start, FVector3 Direction, float Length, float HeadSize = 0.2f)
        => CDebugDrawLibrary.DrawSolidArrow(Engine.World, Start, Direction, Length, Color, HeadSize, FillMode, Duration);

    /// <summary>Screen-space text for this frame (stacked top-left).</summary>
    public static void Text(string Message)
        => CDebugDrawLibrary.DrawText(Engine.World, Message, Color);
}
