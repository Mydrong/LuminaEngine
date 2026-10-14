#include "RuntimePCH.h"
#include "ManagedTypeRegistry.h"

#include "Core/Profiler/Profile.h"
#include "Log/Log.h"

namespace Lumina::Scripting
{
    FManagedTypeRegistry& FManagedTypeRegistry::Get()
    {
        static FManagedTypeRegistry Instance;
        return Instance;
    }

    void FManagedTypeRegistry::Register(IManagedTypeCompiler* Compiler)
    {
        if (Compiler == nullptr)
        {
            return;
        }

        // Idempotent, so installing the built-in set twice leaves one of each rather than running them twice.
        for (const IManagedTypeCompiler* Existing : Stages)
        {
            if (Existing == Compiler)
            {
                return;
            }
        }

        Stages.push_back(Compiler);
    }

    void FManagedTypeRegistry::PreUnloadAll()
    {
        LUMINA_PROFILE_SCOPE();

        for (size_t Index = Stages.size(); Index > 0; --Index)
        {
            Stages[Index - 1]->PreUnload();
        }
    }

    void FManagedTypeRegistry::UnloadAbortedAll()
    {
        LUMINA_PROFILE_SCOPE();

        for (IManagedTypeCompiler* Stage : Stages)
        {
            Stage->UnloadAborted();
        }
    }

    void FManagedTypeRegistry::CompileAll(TSpan<const FManagedTypeDefinition> Definitions, TSpan<const FManagedTypeAlias> Aliases)
    {
        LUMINA_PROFILE_SCOPE();

        for (IManagedTypeCompiler* Stage : Stages)
        {
            Stage->Compile(Definitions, Aliases);
        }
    }
}
