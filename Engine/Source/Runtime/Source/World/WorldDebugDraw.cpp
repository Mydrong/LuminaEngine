#include "RuntimePCH.h"

#include "World/World.h"

namespace Lumina
{
    void CWorld::DrawLine(FVector3 Start, FVector3 End, FVector4 Color,
        float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawLine(Start, End, Color, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawSphere(FVector3 Center, float Radius, FVector4 Color,
        float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSphere(Center, Radius, Color, 16, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
        FVector4 Color, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawBox(Center, HalfExtents, Rotation, Color, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawBoxCorners(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
        FVector4 Color, float CornerFraction, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawBoxCorners(Center, HalfExtents, Rotation, Color, CornerFraction, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawCapsule(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
        float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawCapsule(Start, End, Radius, Color, 16, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawCone(FVector3 Apex, FVector3 Direction, float AngleRadians, float Length,
        FVector4 Color, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawCone(Apex, Direction, AngleRadians, Length, Color, 16, 4, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawArrow(FVector3 Start, FVector3 Direction, float Length, FVector4 Color,
        float HeadSize, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawArrow(Start, Direction, Length, Color, Thickness, bDepthTest, Duration, HeadSize);
        #endif
    }

    void CWorld::DrawCircle(FVector3 Center, FVector3 Normal, float Radius, FVector4 Color,
        uint8 Segments, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawCircle(Center, Normal, Radius, Color, Segments, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawArc(FVector3 Center, FVector3 Normal, float Radius, float StartAngle,
        float SweepAngle, FVector4 Color, uint8 Segments, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawArc(Center, Normal, Radius, StartAngle, SweepAngle, Color, Segments, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawCylinder(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
        uint8 Segments, float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawCylinder(Start, End, Radius, Color, Segments, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawAxes(FVector3 Origin, FQuat Rotation, float Length,
        float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawAxes(Origin, Rotation, Length, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawPoint(FVector3 Position, float Size, FVector4 Color,
        float Thickness, float Duration, bool bDepthTest)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawPoint(Position, Size, Color, Thickness, bDepthTest, Duration);
        #endif
    }

    void CWorld::DrawSolidTriangles(const SDebugDrawVertex* Vertices, int32 Count,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        if (Vertices != nullptr && Count > 0 && Count % 3 == 0)
        {
            DebugInterface.DrawSolidTriangles(TSpan<const SDebugDrawVertex>(Vertices, (size_t)Count), Mode, Duration);
        }
        #endif
    }

    void CWorld::DrawSolidTriangle(FVector3 A, FVector3 B, FVector3 C, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidTriangle(A, B, C, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidQuad(FVector3 A, FVector3 B, FVector3 C, FVector3 D, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidQuad(A, B, C, D, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidBox(Center, HalfExtents, Rotation, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidSphere(FVector3 Center, float Radius, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidSphere(Center, Radius, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidCylinder(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidCylinder(Start, End, Radius, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidCone(FVector3 Apex, FVector3 Direction, float AngleRadians, float Length,
        FVector4 Color, EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidCone(Apex, Direction, AngleRadians, Length, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidCapsule(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidCapsule(Start, End, Radius, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidCircle(FVector3 Center, FVector3 Normal, float Radius, FVector4 Color,
        EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidCircle(Center, Normal, Radius, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidArc(FVector3 Center, FVector3 Normal, float Radius, float StartAngle,
        float SweepAngle, FVector4 Color, EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidArc(Center, Normal, Radius, StartAngle, SweepAngle, Color, Mode, Duration);
        #endif
    }

    void CWorld::DrawSolidArrow(FVector3 Start, FVector3 Direction, float Length, FVector4 Color,
        float HeadSize, EDebugFillMode Mode, float Duration)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawSolidArrow(Start, Direction, Length, Color, HeadSize, Mode, Duration);
        #endif
    }

    void CWorld::DrawText(const FString& Text, FVector4 Color)
    {
        #if !defined(LE_SHIPPING)
        DebugInterface.DrawText(Text, Color);
        #endif
    }
}
