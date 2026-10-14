#pragma once

#include "AssetManager.h"
#include "Containers/String.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Object/SubclassOf.h"
#include "Scripting/ScriptCallback.h"
#include "AssetLibrary.generated.h"

namespace Lumina
{
    // The plumbing under C#'s FAssetManager and FAssetHandle. Handles and objects cross as numbers, so C# wraps each once as its own type.
    REFLECT()
    class RUNTIME_API CAssetLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        // An owned FAssetHandle that Release frees, or zero when nothing resolved.
        FUNCTION(ScriptInternal)
        static uint64 Load(const FString& Path, const FName& Bundle);

        FUNCTION(ScriptInternal)
        static uint64 LoadByName(const FName& Group, const FName& Name, const FName& Bundle);

        FUNCTION(ScriptInternal)
        static uint64 LoadGroup(const FName& Group, const FName& Bundle);

        // Safe from any thread, since what the handle held is let go on the game thread.
        FUNCTION(ScriptInternal)
        static void Release(uint64 Handle);

        FUNCTION(ScriptInternal)
        static bool IsReady(uint64 Handle);

        FUNCTION(ScriptInternal)
        static float GetProgress(uint64 Handle);

        FUNCTION(ScriptInternal)
        static int32 Num(uint64 Handle);

        // Zero unless the object is a Class.
        FUNCTION(ScriptInternal)
        static uint64 Get(uint64 Handle, int32 Index, TSubclassOf<CObject> Class);

        FUNCTION(ScriptInternal)
        static void OnReady(uint64 Handle, SScriptCallback Callback);

        // One name per line, since a list of names cannot cross to script in one call.
        FUNCTION(ScriptInternal)
        static FString GetGroupNames(const FName& Group);

        FUNCTION(ScriptInternal)
        static uint64 LoadSynchronous(const FString& Path);

        // Asks the registry, so nothing loads.
        FUNCTION(ScriptInternal)
        static bool Exists(const FString& Path);
    };
}
