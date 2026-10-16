#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "DebugDrawTypes.generated.h"

namespace Lumina
{
    REFLECT()
    enum class EDebugFillMode : uint8
    {
        Translucent,
        Opaque,
        XRay,
    };

    REFLECT()
    struct RUNTIME_API SDebugDrawVertex
    {
        GENERATED_BODY()

        PROPERTY(Hidden)
        FVector3 Position;

        PROPERTY(Hidden)
        FVector4 Color;
    };
}
