#pragma once

#include "World/ECS/Registry.h"



#include "Platform/GenericPlatform.h"
#include "Containers/Vector.h"
#include "Core/Object/ObjectMacros.h"
#include "Physics/PhysicsTypes.h"
#include "Lumina.h"
#include "Core/Math/Quat/Quat.h"
#include "RayCast.generated.h"

namespace Lumina
{
    // Inline capacity for a query's ignore list; going past it spills to the heap, never drops bodies.
    inline constexpr size_t MaxInlineIgnoreEntities = 8;

    inline constexpr ECollisionProfiles AllCollisionProfiles = ECollisionProfiles::All;

    REFLECT()
    struct SPhysicsBodyTarget
    {
        GENERATED_BODY()

        PROPERTY()
        uint64 Scene = 0;

        PROPERTY()
        uint32 Slot = Constants::kIndexNoneU32;

        PROPERTY()
        uint32 Generation = 0;

        bool IsSet() const { return Scene != 0 && Slot != Constants::kIndexNoneU32; }
    };

    REFLECT()
    enum class EQueryShapeType : uint8
    {
        Sphere,
        Box,
        Capsule,
    };

    // The volume a sweep or overlap tests, centered on the query position, a capsule running along its rotated up axis.
    REFLECT()
    struct SQueryShape
    {
        GENERATED_BODY()

        PROPERTY()
        EQueryShapeType Type = EQueryShapeType::Sphere;

        // Meters, for a sphere and a capsule's caps.
        PROPERTY()
        float Radius = 0.0f;

        // Half a capsule's straight section, so its full height is double the sum of HalfHeight and Radius.
        PROPERTY()
        float HalfHeight = 0.0f;

        PROPERTY()
        FVector3 HalfExtents = FVector3(0.0f);

        PROPERTY()
        FQuat Rotation = FQuat::Identity();

        static SQueryShape Sphere(float Radius)
        {
            SQueryShape Shape;
            Shape.Radius = Radius;
            return Shape;
        }

        static SQueryShape Box(const FVector3& HalfExtents, const FQuat& Rotation)
        {
            SQueryShape Shape;
            Shape.Type = EQueryShapeType::Box;
            Shape.HalfExtents = HalfExtents;
            Shape.Rotation = Rotation;
            return Shape;
        }

        static SQueryShape Capsule(float Radius, float HalfHeight, const FQuat& Rotation)
        {
            SQueryShape Shape;
            Shape.Type = EQueryShapeType::Capsule;
            Shape.Radius = Radius;
            Shape.HalfHeight = HalfHeight;
            Shape.Rotation = Rotation;
            return Shape;
        }
    };

    REFLECT()
    struct SRayResult
    {
        GENERATED_BODY()

        PROPERTY()
        SPhysicsBodyTarget Target;

        PROPERTY()
        uint32 Entity = ECS::NullEntity.Value;

        PROPERTY()
        FVector3 Start;

        PROPERTY()
        FVector3 End;

        PROPERTY()
        FVector3 Location;

        PROPERTY()
        FVector3 Normal;

        /** Normalized distance along ray (0 = start, 1 = end). */
        PROPERTY()
        float Fraction;

        PROPERTY()
        float Distance;

        /** Skeleton bone the hit body belongs to (ragdoll per-bone bodies); Constants::kIndexNone otherwise. */
        PROPERTY()
        int32 BoneIndex = Constants::kIndexNone;

        // A miss keeps the requested Start and End but has no hit data.
        PROPERTY()
        bool bHit = false;
    };

    REFLECT()
    struct SRayCastSettings
    {
        GENERATED_BODY()

        PROPERTY()
        FVector3 Start = FVector3(0.0f);

        PROPERTY()
        FVector3 End = FVector3(0.0f);

        PROPERTY()
        bool bDrawDebug = false;

        /** Seconds; 0 = one frame. */
        PROPERTY()
        float DebugDuration = 0.0f;
        
        PROPERTY()
        bool bIgnoreSelf = false;

        PROPERTY()
        FVector3 DebugHitColor = FVector3(0.0, 1.0f, 0.0f);

        PROPERTY()
        FVector3 DebugMissColor = FVector3(1.0f, 0.0f, 0.0f);

        PROPERTY()
        ECollisionProfiles LayerMask = AllCollisionProfiles;

        PROPERTY()
        TFixedVector<ECS::FEntity, MaxInlineIgnoreEntities> IgnoreEntities;

        FUNCTION()
        void AddIgnoredEntity(ECS::FEntity Entity)
        {
            IgnoreEntities.push_back(Entity);
        }
    };

    REFLECT()
    struct SSphereCastSettings
    {
        GENERATED_BODY()

        PROPERTY()
        FVector3 Start = FVector3(0.0f);

        PROPERTY()
        FVector3 End = FVector3(0.0f);

        /** Sphere radius (meters). */
        PROPERTY()
        float Radius = 0.0f;

        PROPERTY()
        bool bDrawDebug = false;

        /** Seconds; 0 = one frame. */
        PROPERTY()
        float DebugDuration = 0.0f;

        PROPERTY()
        FVector3 DebugHitColor = FVector3(0.0, 1.0f, 0.0f);

        PROPERTY()
        FVector3 DebugMissColor = FVector3(1.0f, 0.0f, 0.0f);

        PROPERTY()
        ECollisionProfiles LayerMask = AllCollisionProfiles;

        PROPERTY()
        TFixedVector<ECS::FEntity, MaxInlineIgnoreEntities> IgnoreEntities;

        FUNCTION()
        void AddIgnoredEntity(ECS::FEntity Entity)
        {
            IgnoreEntities.push_back(Entity);
        }
    };
}
