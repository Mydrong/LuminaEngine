#pragma once

#include "Core/Object/PropertyFlags.h"
#include "Core/Reflection/Type/Function.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class CClass;
    class CEnum;
    class CStruct;
    struct FStructOps;
    struct FVectorOps;
    struct FMapOps;
    struct FOptionalOps;

    struct FMetadataEntry
    {
        const char* Key;
        const char* Value;
    };

    using FPropertyGetter = void (*)(const void* Container, void* OutValue);
    using FPropertySetter = void (*)(void* Container, const void* InValue);

    // What a property holds. One per C++ type, shared by every property of that type.
    struct FPropertyType
    {
        EPropertyTypeFlags Kind = EPropertyTypeFlags::None;

        // Trivial and Builtin belong to the type, so every property of it carries them.
        EPropertyFlags Flags = EPropertyFlags::None;

        // Called at registration rather than when the descriptor is built, so a type may name one registered after it.
        union
        {
            CStruct*            (*Struct)();
            CClass*             (*Class)();
            CEnum*              (*Enum)();
            const FVectorOps*   (*Vector)();
            const FMapOps*      (*Map)();
            const FOptionalOps* (*Optional)();
        } Link{};

        // A type built at runtime already holds what it names, so it points at it instead of linking.
        const void* Resolved = nullptr;

        // An array element, a map's key and value, an optional's payload, an enum's integer or a delegate's arguments.
        const FPropertyType* const* Inner = nullptr;

        // Where each delegate argument sits in its pack, since every other inner shares its owner's storage.
        const uint32* InnerOffsets = nullptr;

        uint8 NumInner = 0;

        NODISCARD CStruct* GetStruct() const                 { return Resolved ? (CStruct*)Resolved : Link.Struct ? Link.Struct() : nullptr; }
        NODISCARD CClass* GetClass() const                   { return Resolved ? (CClass*)Resolved : Link.Class ? Link.Class() : nullptr; }
        NODISCARD CEnum* GetEnum() const                     { return Resolved ? (CEnum*)Resolved : Link.Enum ? Link.Enum() : nullptr; }
        NODISCARD const FVectorOps* GetVectorOps() const     { return Resolved ? (const FVectorOps*)Resolved : Link.Vector ? Link.Vector() : nullptr; }
        NODISCARD const FMapOps* GetMapOps() const           { return Resolved ? (const FMapOps*)Resolved : Link.Map ? Link.Map() : nullptr; }
        NODISCARD const FOptionalOps* GetOptionalOps() const { return Resolved ? (const FOptionalOps*)Resolved : Link.Optional ? Link.Optional() : nullptr; }
    };

    // One reflected member, call-frame slot or container inner.
    struct FPropertyDesc
    {
        const char*           Name = nullptr;
        uint32                Offset = 0;
        EPropertyFlags        Flags = EPropertyFlags::None;
        const FPropertyType*  Type = nullptr;
        FPropertyGetter       Getter = nullptr;
        FPropertySetter       Setter = nullptr;
        const FMetadataEntry* Metadata = nullptr;
        uint16                NumMetadata = 0;
        EPropertyEditorFlags  EditorFlags = EPropertyEditorFlags::None;
    };

    struct FFunctionDesc
    {
        const char*               Name = nullptr;
        EFunctionFlags            Flags = EFunctionFlags::None;
        const FPropertyDesc*      Params = nullptr;
        uint16                    NumParams = 0;

        // Index of the return value among Params, or -1 for a void function.
        int16                     ReturnIndex = -1;

        // sizeof the generated frame struct.
        uint16                    FrameSize = 0;
        FFunction::FNativeFuncPtr Thunk = nullptr;
    };

    struct FStructDesc
    {
        const char*           Name = nullptr;
        CStruct*              (*Super)() = nullptr;
        FStructOps*           (*MakeOps)() = nullptr;
        uint32                Size = 0;
        uint32                Alignment = 0;
        const FPropertyDesc*  Properties = nullptr;
        uint32                NumProperties = 0;
        const FFunctionDesc*  Functions = nullptr;
        uint32                NumFunctions = 0;
        const FMetadataEntry* Metadata = nullptr;
        uint16                NumMetadata = 0;
    };

    struct FClassDesc
    {
        CClass*               (*StaticClass)() = nullptr;
        const FPropertyDesc*  Properties = nullptr;
        uint32                NumProperties = 0;
        const FFunctionDesc*  Functions = nullptr;
        uint32                NumFunctions = 0;
        const FMetadataEntry* Metadata = nullptr;
        uint16                NumMetadata = 0;
    };

    struct FEnumeratorDesc
    {
        const char* Name;
        int64       Value;
    };

    struct FEnumDesc
    {
        const char*            Name = nullptr;
        const FEnumeratorDesc* Enumerators = nullptr;
        uint32                 NumEnumerators = 0;
        const FMetadataEntry*  Metadata = nullptr;
        uint16                 NumMetadata = 0;
    };

    // Declared only, so a property can name a forward-declared type or an alias, and each generated file defines its own.
    template<typename T> CStruct* StaticStruct();
    template<typename T> CClass* StaticClass();
    template<typename T> CEnum* StaticEnum();

    class CObject;
    class CField;
    class CPackage;
    class CScriptClass;
    class CScriptEnum;
    class CScriptStruct;

    // The classes the reflection system is built from, which IMPLEMENT_INTRINSIC_CLASS defines rather than generated code.
    template<> RUNTIME_API CClass* StaticClass<CObject>();
    template<> RUNTIME_API CClass* StaticClass<CField>();
    template<> RUNTIME_API CClass* StaticClass<CStruct>();
    template<> RUNTIME_API CClass* StaticClass<CClass>();
    template<> RUNTIME_API CClass* StaticClass<CEnum>();
    template<> RUNTIME_API CClass* StaticClass<CPackage>();
    template<> RUNTIME_API CClass* StaticClass<CScriptClass>();
    template<> RUNTIME_API CClass* StaticClass<CScriptEnum>();
    template<> RUNTIME_API CClass* StaticClass<CScriptStruct>();
}
