#include "RuntimePCH.h"
#include "ParticleCompileQueue.h"
#include "ParticleSystem.h"

namespace Lumina
{
    FParticleCompileQueue::FParticleCompileQueue() = default;
    FParticleCompileQueue::~FParticleCompileQueue() = default;

    void FParticleCompileQueue::QueueRecompile(CParticleSystem* System)
    {
        FScopeLock Lock(Mutex);
        if (Queued.insert(System).second)
        {
            Recompiles.push_back(System);
        }
    }

    TStrongObjectPtr<CParticleSystem> FParticleCompileQueue::PopRecompile()
    {
        FScopeLock Lock(Mutex);
        if (Recompiles.empty())
        {
            return nullptr;
        }
        TStrongObjectPtr<CParticleSystem> System = Move(Recompiles.back());
        Recompiles.pop_back();
        Queued.erase(System.Get());
        return System;
    }
}
