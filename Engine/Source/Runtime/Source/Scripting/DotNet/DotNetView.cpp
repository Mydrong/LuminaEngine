#include "DotNetExport.h"
#include "ExportSignature.h"
#include "Memory/Construct.h"
#include "World/ECS/Registry.h"
#include "Containers/Vector.h"
#include "Memory/Memory.h"
#include "World/World.h"
#include "World/Entity/Components/Component.h"
#include "World/Entity/Systems/SystemContext.h"

// One boundary crossing per CHUNK rather than per entity, rebinding a reused wrapper handle.

using namespace Lumina;

namespace
{
    ECS::FRegistry* LmViewRegistryFromWorld(uint64 World)
    {
        CWorld* W = DotNet::AsWorld(World);
        return W ? &W->GetEntityRegistry() : nullptr;
    }

    // Snapshotting rather than holding a live iterator is what makes the view mutation-safe.
    struct FViewState
    {
        // The driver's dense entities as ViewBegin saw them, walked back to front like a native view.
        TVector<ECS::FEntity>      Entities;
        size_t                     Remaining = 0;
        const ECS::FSparseSet*     Driver = nullptr;
        int32                      DriverInclude = 0;
        TVector<ECS::FSparseSet*>  IncludeStorages;
        TVector<ECS::FSparseSet*>  ExcludeStorages;
    };
}

// The runtime view is consumed here, and only the snapshot and storages live on.
LUMINA_DOTNET_EXPORT(void*, ViewBegin)(uint64 World, const void* const* IncludeOps, int NInc, const void* const* ExcludeOps, int NExc)
{
    ECS::FRegistry* Registry = LmViewRegistryFromWorld(World);
    if (Registry == nullptr || IncludeOps == nullptr || NInc <= 0)
    {
        return nullptr;
    }

    FViewState* State = Memory::ConstructAt(static_cast<FViewState*>(Memory::Malloc(sizeof(FViewState), alignof(FViewState))));
    State->IncludeStorages.reserve(NInc);

    for (int i = 0; i < NInc; ++i)
    {
        const FComponentOps* Ops = static_cast<const FComponentOps*>(IncludeOps[i]);
        ECS::FSparseSet* Storage = Ops ? Registry->FindStorage(static_cast<uint32>(Ops->TypeId)) : nullptr;
        if (Storage == nullptr)
        {
            // A never-emplaced (or unknown) include type -> the view is empty (no snapshot).
            State->IncludeStorages.clear();
            return State;
        }
        State->IncludeStorages.push_back(Storage);

        // The smallest pool drives, as FRuntimeView picks it.
        if (State->Driver == nullptr || Storage->Num() < State->Driver->Num())
        {
            State->Driver = Storage;
            State->DriverInclude = i;
        }
    }

    for (int i = 0; i < NExc; ++i)
    {
        const FComponentOps* Ops = ExcludeOps ? static_cast<const FComponentOps*>(ExcludeOps[i]) : nullptr;
        if (Ops != nullptr)
        {
            if (ECS::FSparseSet* Storage = Registry->FindStorage(static_cast<uint32>(Ops->TypeId)))
            {
                State->ExcludeStorages.push_back(Storage);
            }
        }
    }

    // Matching waits for the chunk walk, which looks every row up anyway, so the snapshot is one copy.
    const ECS::FEntity* Dense = State->Driver->GetDenseData();
    State->Entities.assign(Dense, Dense + State->Driver->GetDenseSize());
    State->Remaining = State->Entities.size();
    return State;
}

// Pointers resolve fresh per chunk, so a storage realloc between chunks is harmless.
LUMINA_DOTNET_EXPORT(int, ViewNextChunk)(void* StatePtr, uint32* OutEntities, void** OutPtrs, int MaxCount, int NInclude)
{
    FViewState* State = static_cast<FViewState*>(StatePtr);
    if (State == nullptr || OutEntities == nullptr || OutPtrs == nullptr || MaxCount <= 0 || State->Driver == nullptr)
    {
        return 0;
    }

    const int N = (int)State->IncludeStorages.size();
    const int K = NInclude < N ? NInclude : N;
    ECS::FSparseSet* const* Includes = State->IncludeStorages.data();
    ECS::FSparseSet* const* Excludes = State->ExcludeStorages.data();
    const size_t NumExcludes = State->ExcludeStorages.size();
    const ECS::FEntity* Snapshot = State->Entities.data();

    const ECS::FSparseSet* Driver = State->Driver;
    const int32 DriverInclude = State->DriverInclude;
    const ECS::FEntity* DriverDense = Driver->GetDenseData();
    const size_t DriverSize = Driver->GetDenseSize();

    // An unpaged driver's payload is one block, so a row still in its snapshot slot costs a multiply.
    const bool bDriverBlock = !Driver->IsTagOnly() && !Driver->IsPaged();
    uint8* const DriverBlock = Driver->GetPackedBlock();
    const size_t DriverStride = Driver->GetElementSize();

    // A local, since the row writes could alias State and would pin the cursor in memory.
    size_t Remaining = State->Remaining;
    int Count = 0;
    while (Count < MaxCount && Remaining > 0)
    {
        const size_t Slot = --Remaining;
        const ECS::FEntity Entity = Snapshot[Slot];
        if (Entity.IsTombstone())
        {
            continue;
        }

        // Written before the match is known, since a row the loop rejects is simply overwritten by the next.
        void** Row = OutPtrs + (size_t)Count * (size_t)NInclude;

        void* DriverPayload = nullptr;
        if (Slot < DriverSize && DriverDense[Slot] == Entity)
        {
            DriverPayload = bDriverBlock ? DriverBlock + Slot * DriverStride : Driver->GetRawAtDense((uint32)Slot);
        }
        else
        {
            DriverPayload = Driver->GetRaw(Entity);
            if (DriverPayload == nullptr && !(Driver->IsTagOnly() && Driver->Contains(Entity)))
            {
                continue;
            }
        }
        if (DriverInclude < K)
        {
            Row[DriverInclude] = DriverPayload;
        }

        bool bMatches = true;
        for (int k = 0; k < N && bMatches; ++k)
        {
            if (k == DriverInclude)
            {
                continue;
            }
            const ECS::FSparseSet* Storage = Includes[k];
            void* Payload = nullptr;
            if (Storage->IsTagOnly())
            {
                bMatches = Storage->Contains(Entity);
            }
            else
            {
                Payload = Storage->GetRaw(Entity);
                bMatches = Payload != nullptr;
            }
            if (k < K)
            {
                Row[k] = Payload;
            }
        }
        for (size_t e = 0; e < NumExcludes && bMatches; ++e)
        {
            bMatches = !Excludes[e]->Contains(Entity);
        }
        if (!bMatches)
        {
            continue;
        }

        OutEntities[Count] = static_cast<uint32>(Entity.Value);
        ++Count;
    }

    State->Remaining = Remaining;
    return Count;
}

// Each include's layout epoch, so a handle the view hands out can tell when its pool has moved.
LUMINA_DOTNET_EXPORT(void, ViewIncludeEpochs)(void* StatePtr, const uint32** OutSlots, int NInclude)
{
    const FViewState* State = static_cast<const FViewState*>(StatePtr);
    for (int i = 0; i < NInclude; ++i)
    {
        const bool bKnown = State != nullptr && i < (int)State->IncludeStorages.size();
        OutSlots[i] = bKnown ? State->IncludeStorages[i]->GetLayoutEpoch() : nullptr;
    }
}

// Frees the per-call view state allocated by ViewBegin.
LUMINA_DOTNET_EXPORT(void, ViewEnd)(void* StatePtr)
{
    FViewState* State = static_cast<FViewState*>(StatePtr);
    if (State != nullptr)
    {
        State->~FViewState();
        Memory::Free(State);
    }
}

LUMINA_DOTNET_SIGNATURES(
    LUMINA_DOTNET_SIG(ViewBegin),
    LUMINA_DOTNET_SIG(ViewNextChunk),
    LUMINA_DOTNET_SIG(ViewIncludeEpochs),
    LUMINA_DOTNET_SIG(ViewEnd)
);
