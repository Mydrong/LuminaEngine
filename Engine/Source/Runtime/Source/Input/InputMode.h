#pragma once

#include "Core/Object/ObjectMacros.h"
#include "InputMode.generated.h"

namespace Lumina
{
    REFLECT()
    enum class EInputMode : uint8
    {
        Game,
        UI,
        GameAndUI,
    };
}
