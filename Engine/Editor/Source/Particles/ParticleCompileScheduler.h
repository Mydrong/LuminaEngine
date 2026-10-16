#pragma once

#include "Containers/Vector.h"
#include "Memory/SmartPtr.h"

namespace Lumina
{
    struct FParticleEmitterCompileJob;

    // Drains FParticleCompileQueue, building every stale emitter of a system in the background and applying them together.
    class EDITOR_API FParticleCompileScheduler
    {
    public:
        FParticleCompileScheduler();
        ~FParticleCompileScheduler();

        FParticleCompileScheduler(const FParticleCompileScheduler&) = delete;
        FParticleCompileScheduler& operator=(const FParticleCompileScheduler&) = delete;
        FParticleCompileScheduler(FParticleCompileScheduler&&) = delete;
        FParticleCompileScheduler& operator=(FParticleCompileScheduler&&) = delete;

        void Tick();

    private:
        struct FJob;

        void FinishSettledJobs();
        void LaunchJobs();
        void Finish(FJob& Job);

        TVector<TUniquePtr<FJob>> InFlight;

        // Reported together when the queue drains, rather than one toast per system.
        uint32                    AssetsNeedingSave = 0;
    };
}
