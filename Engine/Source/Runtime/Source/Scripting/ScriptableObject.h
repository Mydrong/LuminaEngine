#pragma once

#include "Core/Reflection/Type/Function.h"

#include "Core/Object/Object.h"
#include "Core/Object/ScriptClass.h"
#include "ManagedTypeRegistry.h"

#include "Containers/HashTable.h"
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class CObject;
    class CClass;

    namespace Scriptable
    {
        // The managed instance of a C# subclass, created on first use and null for the CDO or a down host. Game thread only.
        RUNTIME_API void* GetOrCreateInstance(CObject* Object);

        /** The managed instance to dispatch event Slot on, or null to leave the native body in place. */
        FORCEINLINE void* GetOverrideInstance(CObject* Object, int32 Slot)
        {
            const CScriptClass* ScriptClass = ToScriptClass(Object->GetClass());
            return (ScriptClass != nullptr && ScriptClass->HasScriptOverrideSlot(Slot))
                ? GetOrCreateInstance(Object) : nullptr;
        }
    }

    /** One overridable event and the generated thunk that dispatches it into managed code. */
    struct FScriptableEventThunk
    {
        const char*                Name = nullptr;
        FFunction::FNativeFuncPtr  Thunk = nullptr;
    };

    struct FScriptableNativeInfo
    {
        CClass*  (*GetBaseClass)() = nullptr;          // the native Scriptable class's StaticClass()
        CObject* (*Factory)(void* Memory) = nullptr;   // placement-new the shim into Memory
        uint32   ShimSize = 0;
        uint32   ShimAlign = 0;

        // Per event, so an override dispatches through a typed generated call rather than reflection.
        TVector<FScriptableEventThunk> EventThunks;
    };

    struct RUNTIME_API FScriptableRegistry
    {
        static void RegisterNative(const char* NativeClassName, const FScriptableNativeInfo& Info);

        static CScriptClass* Mint(FStringView TypeName, FStringView NativeBaseName,
                                  TSpan<const FString> OverriddenEvents = {});

        // Rebuilds the minted class's overrides from the events the C# type declares it overrides.
        static void ApplyScriptOverrides(CScriptClass* Minted, TSpan<const FString> OverriddenEvents);

        static void RefreshMintedClasses(TSpan<const Scripting::FManagedTypeDefinition> Definitions, TSpan<const Scripting::FManagedTypeAlias> Aliases = {});

        // Mints a class object for every C# class not seen before, without a layout, so other types can name it first.
        static void MintNewClasses(TSpan<const Scripting::FManagedTypeDefinition> Definitions);

        // A class by name, including one minted this reload that has not finished registering yet.
        static CClass* FindClass(const FName& Name);

        // An [Alias] from a C# type, kept name to name since the type it names may not be minted yet.
        static void RegisterClassRedirect(const FName& OldName, const FName& NewName);

        // Replaces every redirect, first in a reload so each later stage can follow a rename.
        static void SetRedirects(TSpan<const Scripting::FManagedTypeAlias> Aliases);

        // The class a stored name means after following rename redirects, or null when nothing answers to it.
        static CClass* ResolveClass(const FName& Name);

        // The struct a stored name means, script structs included, after following rename redirects.
        static CStruct* ResolveStruct(const FName& Name);

        // Whether a chain of redirects leads from OldName to NewName.
        static bool IsRenamedTo(const FName& OldName, const FName& NewName);

        // Every minted class whose name redirects elsewhere, so a reload can move its instances across.
        static void GatherRenamedClasses(THashSet<CClass*>& Out);
    };
}
