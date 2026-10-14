#include "RuntimePCH.h"
#include "ScriptTypeRegistry.h"
#include "ScriptStruct.h"
#include "ScriptableObject.h"
#include "ManagedTypeRegistry.h"
#include "Core/Object/Class.h"
#include "Core/Object/ScriptEnum.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Core/Object/ObjectUtils.h"
#include "Core/Object/Package/Package.h"
#include "Containers/StringFormat.h"
#include "Scripting/DotNet/DotNetHost.h"
#include "Core/Object/InstancedStruct.h"
#include "World/Entity/Components/Component.h"

namespace Lumina
{
    namespace
    {
        template<typename TType>
        TStrongObjectPtr<TType> AllocateTransientType(const FName& Name)
        {
            FConstructCObjectParams Params(TType::StaticClass());
            Params.Name    = Name;
            Params.Flags   = OF_Transient;
            Params.Package = CPackage::GetTransientPackage();
            Params.Guid    = FGuid::New();

            TStrongObjectPtr<TType> Allocated = static_cast<TType*>(StaticAllocateObject(Params));
            if (Allocated.Get() != nullptr)
            {
                FinishRegistration(Allocated.Get());
            }
            return Allocated;
        }

        // Only a type a C# reload produced may be replaced, so a native name is never taken over.
        bool IsClaimedByNativeType(const FName& Name)
        {
            const CField* Existing = FindObject<CField>(Name);
            return Existing != nullptr && Cast<CScriptStruct>(Existing) == nullptr && Cast<CScriptEnum>(Existing) == nullptr;
        }

        NODISCARD bool IsComponent(const Scripting::FManagedTypeDefinition& Desc)
        {
            for (const std::pair<FString, FString>& Pair : Desc.Meta)
            {
                if (Pair.first == "Component")
                {
                    return true;
                }
            }
            return false;
        }

        const CStruct& StructOf(const ECS::FComponentTypeInfo& Info)
        {
            return *Info.GetBoundStruct();
        }

        void ConstructComponent(const ECS::FComponentTypeInfo& Info, void* Dest)
        {
            StructOf(Info).InitializeStruct(Dest);
        }

        void CopyConstructComponent(const ECS::FComponentTypeInfo& Info, void* Dest, const void* Source)
        {
            StructOf(Info).InitializeStruct(Dest);
            StructOf(Info).CopyStruct(Dest, Source);
        }

        void DestructComponent(const ECS::FComponentTypeInfo& Info, void* Dest)
        {
            StructOf(Info).DestroyStruct(Dest);
        }

        void RelocateComponent(const ECS::FComponentTypeInfo& Info, void* Dest, void* Source)
        {
            CopyConstructComponent(Info, Dest, Source);
            DestructComponent(Info, Source);
        }

        void MigrateComponent(const ECS::FComponentTypeInfo& From, const void* FromValue, const ECS::FComponentTypeInfo& To, void* ToValue)
        {
            MigrateStructByFieldName(From.GetBoundStruct(), FromValue, To.GetBoundStruct(), ToValue);
        }

        // Paged like a native component that asks for nothing, so a handle into a pool survives an emplace beside it.
        ECS::FComponentTypeInfo MakeComponentTypeInfo(const FName& Name, CScriptStruct& Struct)
        {
            ECS::FComponentTypeInfo Info;
            Info.Name             = Name;
            Info.CachedStruct     = &Struct;
            Info.Size             = Struct.GetAlignedSize();
            Info.Alignment        = Math::Max<uint32>(Struct.GetAlignment(), 1u);
            Info.bEmpty           = Info.Size == 0;
            Info.bPaged           = !Info.bEmpty;
            Info.bRuntimeLayout   = true;
            Info.DefaultConstruct = &ConstructComponent;
            Info.CopyConstruct    = &CopyConstructComponent;
            Info.Destruct         = &DestructComponent;
            Info.Relocate         = &RelocateComponent;
            return Info;
        }

        // Everything a script struct's metadata holds comes from its C# type, so a specifier removed there is removed here.
        void ApplyScriptMetadata(CScriptStruct& Struct, const Scripting::FManagedTypeDefinition& Desc)
        {
            Struct.Metadata = FMetaDataPair();
            for (const std::pair<FString, FString>& Pair : Desc.Meta)
            {
                Struct.Metadata.SetValue(FName(Pair.first.c_str()), Pair.second);
            }
            // An asset stores this rather than the object's name, which a superseded type no longer has.
            Struct.Metadata.SetValue("ScriptTypeName", Desc.TypeName.ToString());
        }
    }

    FScriptStructRegistry& FScriptStructRegistry::Get()
    {
        static FScriptStructRegistry Instance;
        return Instance;
    }

    FScriptStructRegistry::FScriptStructRegistry() = default;
    FScriptStructRegistry::~FScriptStructRegistry() = default;

    void FScriptStructRegistry::RegisterComponent(const FName& Name, CScriptStruct& Struct)
    {
        ECS::FComponentTypeRegistry& ComponentTypes = ECS::FComponentTypeRegistry::Get();
        const ECS::FComponentTypeInfo Info = MakeComponentTypeInfo(Name, Struct);

        // A type renamed with [Alias] takes over its old pool, so live components follow the rename.
        FComponentOps* Ops = nullptr;
        for (const TUniquePtr<FComponentOps>& Existing : ComponentOps)
        {
            const FName& Held = ComponentTypes.GetInfo(Existing->GetTypeID()).Name;
            if (Held == Name)
            {
                Ops = Existing.Get();
                break;
            }
            if (Ops == nullptr && FScriptableRegistry::IsRenamedTo(Held, Name))
            {
                Ops = Existing.Get();
            }
        }

        if (Ops == nullptr)
        {
            Ops = ComponentOps.emplace_back(MakeUnique<FComponentOps>()).Get();
            Ops->TypeId = ComponentTypes.Acquire(Info);
        }
        else if (ComponentTypes.GetInfo(Ops->GetTypeID()).GetBoundStruct() != &Struct)
        {
            ComponentTypes.Relayout(Ops->GetTypeID(), Info, &MigrateComponent);
        }
        RegisterComponentOps(&Struct, Ops);
    }

    void FScriptStructRegistry::Supersede(TStrongObjectPtr<CScriptStruct>&& Struct)
    {
        CScriptStruct* Old = Struct.Get();
        if (Old == nullptr)
        {
            return;
        }
        Old->MarkSuperseded();
        const FString Retired = FString("REPLACED_") + Old->GetName().c_str();
        Old->Rename(FName(Retired.c_str(), ++SupersededSerial), Old->GetPackage());
        Superseded.push_back(std::move(Struct));
        ++Generation;
    }

    void FScriptStructRegistry::Clear()
    {
        for (auto& [Name, Struct] : Types)
        {
            Supersede(std::move(Struct));
        }
        Types.clear();
        // A value of a type that never reached the registry still has to learn its type is gone.
        ++Generation;
    }

    void FScriptStructRegistry::Refresh(TSpan<const Scripting::FManagedTypeDefinition> Definitions)
    {
        THashSet<FName> Live;
        uint32 MintedCount = 0;

        // Definitions arrive with every struct after the structs it holds by value, so each finds its fields' types already current.
        for (const Scripting::FManagedTypeDefinition& Desc : Definitions)
        {
            if (Desc.Kind != Scripting::EManagedTypeKind::ScriptStruct || Desc.TypeName.IsNone())
            {
                continue;
            }

            const FName Name = Desc.TypeName;
            Live.insert(Name);

            CStruct* Base = nullptr;
            if (!Desc.NativeBaseName.empty())
            {
                Base = FindObject<CStruct>(FName(Desc.NativeBaseName.c_str()));
                if (Base == nullptr)
                {
                    LOG_WARN("Script struct '{}' extends '{}', which is not a reflected struct; not minted.", Name, Desc.NativeBaseName.c_str());
                    continue;
                }
            }

            auto Existing = Types.find(Name);
            if (Existing != Types.end())
            {
                CScriptStruct* Current = Existing->second.Get();
                const EScriptTypeDirty Dirty = Scripting::DiffScriptLayout(Current->GetAppliedSchema(), Current->GetOwnProperties(), Desc.Schema);
                if (Current->GetNativeBase() == Base && !EnumHasAnyFlags(Dirty, EScriptTypeDirty::Layout))
                {
                    // Same layout, so every holder keeps its pointer and only the defaults and metadata are brought up to date.
                    const FString Stable = Name.ToString();
                    DotNet::ApplyScriptableDefaults(FStringView(Stable.c_str(), Stable.size()), Current->GetDefaultInstance());
                    ApplyScriptMetadata(*Current, Desc);
                    Current->SetAppliedSchema(Desc.Schema);
                    if (IsComponent(Desc))
                    {
                        RegisterComponent(Name, *Current);
                    }
                    continue;
                }
                Supersede(std::move(Existing->second));
                Types.erase(Existing);
            }

            if (IsClaimedByNativeType(Name))
            {
                LOG_WARN("Script struct '{}' has the name of a native type; not minted.", Name);
                continue;
            }

            TStrongObjectPtr<CScriptStruct> Minted = AllocateTransientType<CScriptStruct>(Name);
            if (Minted.Get() == nullptr || !Minted->BuildFromSchema(Desc.Schema, Base))
            {
                LOG_WARN("Script struct '{}': layout could not be built; not minted.", Name);
                continue;
            }
            Minted->SetAppliedSchema(Desc.Schema);

            // A C# struct's initializers live in its rewritten accessors, so the schema carried none of them.
            const FString Stable = Name.ToString();
            DotNet::ApplyScriptableDefaults(FStringView(Stable.c_str(), Stable.size()), Minted->GetDefaultInstance());
            ApplyScriptMetadata(*Minted, Desc);
            if (IsComponent(Desc))
            {
                RegisterComponent(Name, *Minted);
            }
            Types[Name] = std::move(Minted);
            ++MintedCount;
        }

        TVector<FName> Removed;
        for (const auto& [Name, Struct] : Types)
        {
            if (Live.find(Name) == Live.end())
            {
                Removed.push_back(Name);
            }
        }
        for (const FName& Name : Removed)
        {
            Supersede(std::move(Types[Name]));
            Types.erase(Name);
        }

        if (MintedCount > 0)
        {
            LOG_INFO("Minted {} script struct(s).", MintedCount);
        }
    }

    CScriptStruct* FScriptStructRegistry::Find(const FName& ScriptTypeName) const
    {
        auto It = Types.find(ScriptTypeName);
        return It != Types.end() ? It->second.Get() : nullptr;
    }

    FScriptEnumRegistry& FScriptEnumRegistry::Get()
    {
        static FScriptEnumRegistry Instance;
        return Instance;
    }

    void FScriptEnumRegistry::Refresh(TSpan<const Scripting::FManagedTypeDefinition> Definitions)
    {
        for (const Scripting::FManagedTypeDefinition& Desc : Definitions)
        {
            if (Desc.Kind != Scripting::EManagedTypeKind::Enum || Desc.TypeName.IsNone())
            {
                continue;
            }

            TStrongObjectPtr<CScriptEnum>& Enum = Enums[Desc.TypeName];
            if (Enum.Get() == nullptr)
            {
                if (IsClaimedByNativeType(Desc.TypeName))
                {
                    LOG_WARN("Script enum '{}' has the name of a native type; not minted.", Desc.TypeName);
                    Enums.erase(Desc.TypeName);
                    continue;
                }
                Enum = AllocateTransientType<CScriptEnum>(Desc.TypeName);
                if (Enum.Get() == nullptr)
                {
                    Enums.erase(Desc.TypeName);
                    continue;
                }
            }

            // A property holds the enum itself, so members change in place and every field sees them at once.
            Enum->UnderlyingType = Desc.EnumUnderlying;
            Enum->Names.clear();
            for (const Scripting::FScriptEnumEntry& Entry : Desc.EnumEntries)
            {
                Enum->AddEnum(Entry.Name, (uint64)Entry.Value);
            }
            Enum->Metadata = FMetaDataPair();
            for (const std::pair<FString, FString>& Pair : Desc.Meta)
            {
                Enum->Metadata.SetValue(FName(Pair.first.c_str()), Pair.second);
            }
        }
    }

    CStruct* ResolveStructByName(const FName& Name)
    {
        return FScriptableRegistry::ResolveStruct(Name);
    }

    FName StructIdentity(const CStruct* Struct)
    {
        if (Struct == nullptr)
        {
            return FName();
        }
        if (const FString* Stable = Struct->Metadata.TryGetMetadata("ScriptTypeName"))
        {
            if (!Stable->empty())
            {
                return FName(Stable->c_str());
            }
        }
        return Struct->GetName();
    }
}
