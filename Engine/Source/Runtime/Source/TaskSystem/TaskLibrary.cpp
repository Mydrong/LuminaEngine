#include "RuntimePCH.h"

#include "TaskLibrary.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskTypes.h"

namespace Lumina
{
    uint64 CTaskLibrary::Run(SScriptCallback Body, int32 Priority)
    {
        if (!Body.IsBound())
        {
            return 0;
        }

        // One-shot, so the invoke frees the handle and nothing has to own it for the task's lifetime.
        FTaskHandle Task = Task::AsyncTask(1, 0, [Body](uint32, uint32, uint32)
        {
            Scripting::InvokeScriptCallback(Body, 0);
        }, static_cast<ETaskPriority>(Priority));

        return (uint64)(uintptr_t)(new FTaskHandle(Task));
    }

    void CTaskLibrary::ParallelFor(uint32 Num, uint32 MinRange, SScriptCallback Body, int32 Priority)
    {
        if (Body.IsBound() && Num > 0)
        {
            Task::ParallelFor(Num, [Body](const Task::FParallelRange& Range)
            {
                Scripting::InvokeScriptCallbackRepeating(Body, (uint64)Range.Start | ((uint64)Range.End << 32));
            }, MinRange, static_cast<ETaskPriority>(Priority));
        }
        Scripting::ReleaseScriptCallback(Body);
    }

    void CTaskLibrary::Wait(uint64 Handle)
    {
        if (Handle != 0)
        {
            (*reinterpret_cast<FTaskHandle*>((uintptr_t)Handle))->Wait();
        }
    }

    void CTaskLibrary::Release(uint64 Handle)
    {
        delete reinterpret_cast<FTaskHandle*>((uintptr_t)Handle);
    }

    void CTaskLibrary::WaitForAll()
    {
        GTaskSystem->WaitForAll();
    }

    int32 CTaskLibrary::NumWorkers()
    {
        return static_cast<int32>(GTaskSystem->GetNumWorkers());
    }
}
