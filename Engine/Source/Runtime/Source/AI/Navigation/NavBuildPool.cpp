#include "RuntimePCH.h"
#include "NavBuildPool.h"

namespace Lumina
{
    FNavBuildPool::FNavBuildPool(uint32 NumThreads)
    {
        Threads.reserve(NumThreads);
        for (uint32 i = 0; i < NumThreads; ++i)
        {
            Threads.emplace_back(FThreadOptions{ "NavBuild", Threading::ThreadGroup_Other, EThreadPriority::Background }, [this] { ThreadLoop(); });
        }
    }

    FNavBuildPool::~FNavBuildPool()
    {
        {
            FScopeLock Lock(QueueLock);
            bStopping = true;
        }
        QueueWake.NotifyAll();
        for (FThread& Thread : Threads)
        {
            Thread.Join();
        }
    }

    void FNavBuildPool::Enqueue(TMoveOnlyFunction<void()>&& Job)
    {
        {
            FScopeLock Lock(QueueLock);
            Queue.push_back(std::move(Job));
        }
        QueueWake.NotifyOne();
    }

    bool FNavBuildPool::RunOne()
    {
        TMoveOnlyFunction<void()> Job;
        {
            FScopeLock Lock(QueueLock);
            if (Queue.empty())
            {
                return false;
            }
            Job = std::move(Queue.front());
            Queue.pop_front();
        }
        Job();
        return true;
    }

    void FNavBuildPool::ThreadLoop()
    {
        while (true)
        {
            TMoveOnlyFunction<void()> Job;
            {
                FUniqueLock Lock(QueueLock);
                QueueWake.Wait(Lock, [this] { return bStopping || !Queue.empty(); });
                if (Queue.empty())
                {
                    return;
                }
                Job = std::move(Queue.front());
                Queue.pop_front();
            }
            Job();
        }
    }
}
