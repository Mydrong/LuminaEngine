#pragma once

#include "Containers/Vector.h"
#include "Containers/String.h"
#include "Containers/Span.h"
#include "Core/Threading/Atomic.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    // Content-addressed, so an entry is never rewritten and caches shared through a folder or a repository cannot conflict.
    class RUNTIME_API FArtifactCache
    {
    public:
        // Reads the artifact stored under Key, from the local cache first and then the shared one, which also seeds the local copy.
        bool Load(uint64 Key, TVector<uint8>& OutData);

        // Writes Data under Key locally, and to the shared cache when the project names one.
        void Store(uint64 Key, TSpan<const uint8> Data);

    private:
        TAtomic<bool> bReportedReadOnlyShare{false};
    };
}
