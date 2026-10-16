#include "RuntimePCH.h"

#include "ECSReferenceProvider.h"

#include "Registry.h"

namespace Lumina
{
    void VisitRegistryObjectReferences(ECS::FRegistry& Registry, FObjectReferenceVisitor::FSlotFunc Func)
    {
        {
            for (ECS::FSparseSet* Storage : Registry.GetActiveStorages())
            {
                if (Storage == nullptr)
                {
                    continue;
                }

                CStruct* const Struct = Storage->GetStruct();
                if (Struct == nullptr)
                {
                    continue;   // an unreflected storage holds no properties to walk
                }

                const ECS::FEntity* Dense = Storage->GetDenseData();
                const size_t   Count = Storage->GetDenseSize();
                for (size_t Index = 0; Index < Count; ++Index)
                {
                    if (Dense[Index].IsTombstone())
                    {
                        continue;
                    }
                    FObjectReferenceVisitor::VisitStruct(Struct, Storage->GetRawAtDense((uint32)Index), Func);
                }
            }
        }
    }
}
