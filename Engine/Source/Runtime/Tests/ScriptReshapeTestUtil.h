#pragma once

#include "Core/Object/Class.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Object/ObjectReinstancer.h"
#include "Scripting/ScriptStruct.h"
#include "Scripting/ScriptableObject.h"

namespace ScriptTest
{
    /**
     * Reshapes a minted class the way a reload now does.
     *
     * An instance's size is fixed at allocation, so a new layout needs a new class and the reinstancer moves
     * the live instances onto it. Returns the replacement, which is what every later lookup must go through.
     */
    inline Lumina::CScriptClass* Reshape(Lumina::CScriptClass* Old, const char* NewName,
        const Lumina::Scripting::FScriptExportSchema& Schema, const char* NativeBase = "CScriptableTest")
    {
        using namespace Lumina;

        CScriptClass* New = FScriptableRegistry::Mint(NewName, NativeBase);
        if (New == nullptr)
        {
            return nullptr;
        }

        Scripting::AppendScriptPropertiesToClass(New, Schema);
        RegisterPendingTypes();
        New->GetDefaultObject();

        FObjectReinstancer Reinstancer;
        Reinstancer.MapClass(Old, New);
        Reinstancer.Commit();
        return New;
    }
}
