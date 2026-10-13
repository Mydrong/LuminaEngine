#include "RuntimePCH.h"
#include "MaterialCompileQueue.h"
#include "Material.h"
#include "Containers/Algorithm.h"

namespace Lumina
{
    FMaterialCompileQueue::FMaterialCompileQueue() = default;
    FMaterialCompileQueue::~FMaterialCompileQueue() = default;

    void FMaterialCompileQueue::QueueRecompile(CMaterial* Material)
    {
        FScopeLock Lock(Mutex);
        if (QueuedRecompiles.insert(Material).second)
        {
            Recompiles.push_back(Material);
        }
    }

    void FMaterialCompileQueue::QueueFunctionRecompile(const FGuid& MaterialGUID)
    {
        FScopeLock Lock(Mutex);
        if (QueuedFunctionRecompiles.insert(MaterialGUID).second)
        {
            FunctionRecompiles.push_back(MaterialGUID);
        }
    }

    void FMaterialCompileQueue::QueuePermutation(CMaterial* Material, uint64 Key)
    {
        // A cooked material has no graph to compile from, and the default permutation always exists.
        if (!IsValid(Material) || Material->GetPackage() == nullptr || Material->HasPermutation(Key))
        {
            return;
        }

        FScopeLock Lock(Mutex);
        const bool bQueued = Algo::AnyOf(Permutations, [Material, Key](const FPermutationRequest& Request)
        {
            return Request.Material == Material && Request.Key == Key;
        });
        if (!bQueued)
        {
            Permutations.push_back({ Material, Key });
        }
    }

    TStrongObjectPtr<CMaterial> FMaterialCompileQueue::PopRecompile()
    {
        FScopeLock Lock(Mutex);
        if (Recompiles.empty())
        {
            return nullptr;
        }
        TStrongObjectPtr<CMaterial> Material = Move(Recompiles.back());
        Recompiles.pop_back();
        QueuedRecompiles.erase(Material.Get());
        return Material;
    }

    bool FMaterialCompileQueue::PopFunctionRecompile(FGuid& OutMaterialGUID)
    {
        FScopeLock Lock(Mutex);
        if (FunctionRecompiles.empty())
        {
            return false;
        }
        OutMaterialGUID = FunctionRecompiles.back();
        FunctionRecompiles.pop_back();
        QueuedFunctionRecompiles.erase(OutMaterialGUID);
        return true;
    }

    bool FMaterialCompileQueue::PopPermutation(TStrongObjectPtr<CMaterial>& OutMaterial, uint64& OutKey)
    {
        FScopeLock Lock(Mutex);
        if (Permutations.empty())
        {
            return false;
        }
        OutMaterial = Move(Permutations.back().Material);
        OutKey      = Permutations.back().Key;
        Permutations.pop_back();
        return true;
    }

    bool FMaterialCompileQueue::IsFunctionRecompileQueued(const FGuid& MaterialGUID) const
    {
        FScopeLock Lock(Mutex);
        return QueuedFunctionRecompiles.contains(MaterialGUID);
    }
}
