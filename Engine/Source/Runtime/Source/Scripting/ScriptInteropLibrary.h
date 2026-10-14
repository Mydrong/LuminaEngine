#pragma once

#include "Containers/String.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "ScriptInteropLibrary.generated.h"

namespace Lumina
{
    // What LuminaSharp asks the engine at startup to prove both sides agree on layouts and enum values.
    REFLECT()
    class RUNTIME_API CScriptInteropLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        // The native size behind a hand-written C# mirror's key, or -1 when nothing registered that key.
        FUNCTION(ScriptInternal)
        static int32 GetMirrorSize(const FString& Key);

        // An EPropertyTypeFlags enumerator's value by its plain name, or -1 when there is none.
        FUNCTION(ScriptInternal)
        static int32 GetPropertyTypeValue(const FString& Name);

        FUNCTION(ScriptInternal)
        static int32 GetPropertyTypeCount();

        // An EPropertyFlags bit by name, or -1 when there is none.
        FUNCTION(ScriptInternal)
        static int32 GetPropertyFlagValue(const FString& Name);

        // Size in the low half and alignment in the high half, or -1 when the kind has no fixed layout.
        FUNCTION(ScriptInternal)
        static int64 GetValueLayout(int32 Kind, const FString& StructName);

        // The layout hash a loaded module registered, or zero when none by that name is loaded.
        FUNCTION(ScriptInternal)
        static uint64 GetModuleLayoutHash(const FString& Module);
    };
}
