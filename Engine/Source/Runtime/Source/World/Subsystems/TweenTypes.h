#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Platform/GenericPlatform.h"
#include "TweenTypes.generated.h"

namespace Lumina
{
    // How a looping tween starts its next pass.
    REFLECT()
    enum class ETweenLoopMode : uint8
    {
        // Jumps back to the start and plays forward again.
        Restart,

        // Plays the sequence backward on every other pass, ending where it began.
        Yoyo,
    };

    // Which part of a transform a punch or shake offsets.
    REFLECT()
    enum class ETweenTransformChannel : uint8
    {
        Location,
        Rotation,
        Scale,
    };
}
