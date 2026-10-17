#pragma once

#include "Platform/GenericPlatform.h"

namespace Lumina
{
    struct FBulkDataRef
    {
        int64 Offset = 0;
        int64 Size = 0;

        bool IsValid() const { return Size > 0; }
    };
}
