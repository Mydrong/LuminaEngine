#pragma once

#include "Containers/SegmentArray.h"

namespace Lumina
{
    struct FShaderEntry;

    // Weak, generational reference to a shader-library entry.
    using FShaderH = THandle<FShaderEntry>;
}
