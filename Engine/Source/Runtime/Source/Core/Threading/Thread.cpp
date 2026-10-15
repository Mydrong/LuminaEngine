#include "RuntimePCH.h"
#include "Thread.h"
#include "Platform/Time/PlatformTime.h"
#include "rpmalloc.h"
#include "Core/Assertions/Assert.h"


namespace Lumina
{
    bool FThreadStop::WaitForStop(uint32 TimeoutMs) const noexcept
    {
        if (!ShouldStop())
        {
            Threading::WaitOnAddress32(reinterpret_cast<const volatile uint32*>(&Word), 0u, TimeoutMs);
        }
        return ShouldStop();
    }

    void FThreadStop::Request() noexcept
    {
        Word.store(1, std::memory_order_release);
        Threading::WakeAllOnAddress32(reinterpret_cast<const volatile uint32*>(&Word));
    }

    void FThread::RequestStop() noexcept
    {
        if (Stop)
        {
            Stop->Request();
        }
    }

    namespace Threading
    {
        static uint64 GMainThreadID = 0;

        void ThreadYield()
        {
            PlatformTime::YieldThread();
        }


        bool IsMainThread()
        {
            return GMainThreadID != 0 && GMainThreadID == GetThreadID();
        }

        void Sleep(uint64 Milliseconds)
        {
            PlatformTime::SleepMilliseconds(static_cast<uint32>(Milliseconds));
        }

        void Initialize(const char* MainThreadName)
        {
            GMainThreadID = GetThreadID();
            SetThreadName(MainThreadName, ThreadGroup_Main);
        }

        void Shutdown()
        {
            GMainThreadID = 0;
        }

        void InitializeThreadHeap()
        {
            rpmalloc_thread_initialize();
        }

        void ShutdownThreadHeap()
        {
            rpmalloc_thread_finalize(1);
        }

    }
}
