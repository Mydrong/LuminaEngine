#pragma once

#include "Containers/HashTable.h"
#include "Containers/Vector.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/Threading/Thread.h"

namespace Lumina
{
    class CParticleSystem;

    // Systems whose emitters loaded with no shader for the current templates, filled from PostLoad and drained by the editor.
    class RUNTIME_API FParticleCompileQueue
    {
    public:
        FParticleCompileQueue();
        ~FParticleCompileQueue();

        void QueueRecompile(CParticleSystem* System);

        TStrongObjectPtr<CParticleSystem> PopRecompile();

    private:
        mutable FMutex                             Mutex;
        TVector<TStrongObjectPtr<CParticleSystem>> Recompiles;
        THashSet<const CParticleSystem*>           Queued;
    };
}
