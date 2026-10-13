#pragma once

#include "Core/Reflection/ReflectionMacros.h"
#include "Memory/Construct.h"
#include "ObjectCore.h"
#include "Lumina.h"


#define DECLARE_CLASS(TNamespace, TClass, TBaseClass, TPackage, TAPI) \
private: \
    friend struct LUMINA_TYPE_INFO(TClass); \
    TNamespace::TClass& operator=(TNamespace::TClass&&); \
    TNamespace::TClass& operator=(const TNamespace::TClass&); \
    TAPI static Lumina::CClass* CreateStaticClass(); \
public: \
    using ThisClass = TNamespace::TClass; \
    using Super = TBaseClass; \
    inline static Lumina::CClass* StaticClass() \
    { \
        static Lumina::CClass* Class = nullptr; \
        if (Class == nullptr) \
        { \
            Class = CreateStaticClass(); \
        } \
        return Class; \
    } \
    inline static Lumina::FName StaticName() \
    { \
        static Lumina::FName StaticName(#TClass); \
        return StaticName; \
    } \
    inline static const TCHAR* StaticPackage() \
    { \
        return TEXT(TPackage); \
    } \

#define DECLARE_SERIALIZER(TNamespace, TClass) \
    public: \
    friend Lumina::FArchive& operator << (Lumina::FArchive& Ar, TNamespace::TClass*& Res) \
    { \
        return Ar << (Lumina::CObject*&)Res; \
    } \
    private:

// Qualified throughout, since a reflected class may live in any namespace or none.
#define DEFINE_CLASS_FACTORY(TClass) \
    static Lumina::CObject* __PlacementNew(void* Memory) \
    { \
        return Lumina::Memory::ConstructAt(static_cast<TClass*>(Memory)); \
    }

// Allocates the class object, which the generated descriptor later fills with properties and functions.
#define IMPLEMENT_CLASS(TNamespace, TClass) \
    NO_API Lumina::CClass* TNamespace::TClass::CreateStaticClass() \
    { \
        static Lumina::CClass* Class = nullptr; \
        if (Class == nullptr) \
        { \
            Lumina::AllocateStaticClass(TNamespace::TClass::StaticPackage(), TEXT(#TClass), &Class, \
                sizeof(TNamespace::TClass), alignof(TNamespace::TClass), \
                &TNamespace::TClass::Super::StaticClass, &TNamespace::TClass::__PlacementNew); \
        } \
        return Class; \
    }

// A class the reflection system is built from, so nothing generates it and it registers itself.
#define IMPLEMENT_INTRINSIC_CLASS(TClass, TBaseClass, TAPI) \
    IMPLEMENT_CLASS(Lumina, TClass) \
    template<> TAPI Lumina::CClass* Lumina::StaticClass<Lumina::TClass>() \
    { \
        static Lumina::CClass* Class = nullptr; \
        if (Class == nullptr) \
        { \
            Lumina::StaticClass<Lumina::TBaseClass>(); \
            Class = Lumina::TClass::StaticClass(); \
            Lumina::FinishRegistration(Class); \
        } \
        return Class; \
    } \
    static constexpr Lumina::FClassConstructor CONCAT(IntrinsicClass_, TClass)[] = { &Lumina::StaticClass<Lumina::TClass> }; \
    static constexpr Lumina::FCompiledInTypes CONCAT(IntrinsicTypes_, TClass){ .Classes = CONCAT(IntrinsicClass_, TClass), .NumClasses = 1 }; \
    static const Lumina::FTypeRegistrar CONCAT(IntrinsicRegistrar_, TClass)(CONCAT(IntrinsicTypes_, TClass));

// The trailing metadata fields of a generated descriptor, whose runtime entries lead so a build without the editor keeps only those.
#if USING(WITH_EDITOR)
    #define LUMINA_METADATA(Array, NumRuntime) , .Metadata = Array, .NumMetadata = (Lumina::uint16)std::size(Array)
#else
    #define LUMINA_METADATA(Array, NumRuntime) , .Metadata = Array, .NumMetadata = NumRuntime
#endif
