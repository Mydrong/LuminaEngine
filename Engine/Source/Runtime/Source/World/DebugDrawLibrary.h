#pragma once

#include "Containers/String.h"
#include "Containers/Span.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Quat/Quat.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "DebugDrawTypes.h"
#include "DebugDrawLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    /** Transient debug geometry for a world. A Duration at or below zero draws for one frame. */
    REFLECT()
    class RUNTIME_API CDebugDrawLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        FUNCTION()
        static void DrawLine(CWorld* World, FVector3 Start, FVector3 End, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawSphere(CWorld* World, FVector3 Center, float Radius, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawBox(CWorld* World, FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
            FVector4 Color, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawBoxCorners(CWorld* World, FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
            FVector4 Color, float CornerFraction = 0.25f, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawCapsule(CWorld* World, FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawCone(CWorld* World, FVector3 Apex, FVector3 Direction, float AngleRadians, float Length,
            FVector4 Color, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawArrow(CWorld* World, FVector3 Start, FVector3 Direction, float Length, FVector4 Color,
            float HeadSize = 0.2f, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawCircle(CWorld* World, FVector3 Center, FVector3 Normal, float Radius, FVector4 Color,
            uint8 Segments = 32, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawArc(CWorld* World, FVector3 Center, FVector3 Normal, float Radius, float StartAngle,
            float SweepAngle, FVector4 Color, uint8 Segments = 16, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawCylinder(CWorld* World, FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            uint8 Segments = 16, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawAxes(CWorld* World, FVector3 Origin, FQuat Rotation, float Length,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawPoint(CWorld* World, FVector3 Position, float Size, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        static void DrawSolidTriangles(CWorld* World, const SDebugDrawVertex* Vertices, int32 Count,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidTriangle(CWorld* World, FVector3 A, FVector3 B, FVector3 C, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidQuad(CWorld* World, FVector3 A, FVector3 B, FVector3 C, FVector3 D, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidBox(CWorld* World, FVector3 Center, FVector3 HalfExtents, FQuat Rotation, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidSphere(CWorld* World, FVector3 Center, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidCylinder(CWorld* World, FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidCone(CWorld* World, FVector3 Apex, FVector3 Direction, float AngleRadians, float Length,
            FVector4 Color, EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidCapsule(CWorld* World, FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidCircle(CWorld* World, FVector3 Center, FVector3 Normal, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidArc(CWorld* World, FVector3 Center, FVector3 Normal, float Radius, float StartAngle,
            float SweepAngle, FVector4 Color, EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        static void DrawSolidArrow(CWorld* World, FVector3 Start, FVector3 Direction, float Length, FVector4 Color,
            float HeadSize = 0.2f, EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        /** One screen-space line, stacked from the top left of the world viewport for this frame only. */
        FUNCTION()
        static void DrawText(CWorld* World, const FString& Text, FVector4 Color);
    };
}
