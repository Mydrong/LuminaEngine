#include "RuntimePCH.h"

#include "GameplayTagLibrary.h"

#include "GameplayTags/GameplayTagComponent.h"
#include "GameplayTags/GameplayTagRegistry.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // The component stores serializable name-backed tags, so an id has to be resolved to one first.
        SGameplayTag TagFromId(uint32 TagId)
        {
            SGameplayTag Tag;
            const FString Name = FGameplayTagRegistry::Get().GetName(TagId);
            if (!Name.empty())
            {
                Tag.TagName = FName(Name.c_str());
            }
            return Tag;
        }

        const SGameplayTagComponent* ComponentOf(CWorld* World, ECS::FEntity Entity, uint32 TagId)
        {
            if (World == nullptr || TagId == 0)
            {
                return nullptr;
            }
            return World->TryGetComponent<SGameplayTagComponent>(Entity);
        }
    }
    void CGameplayTagLibrary::GetTags(CWorld* World, ECS::FEntity Entity, TVector<uint32>& Out)
    {
        if (World == nullptr)
        {
            return;
        }

        const SGameplayTagComponent* Tags = World->TryGetComponent<SGameplayTagComponent>(Entity);
        if (Tags == nullptr)
        {
            return;
        }

        FGameplayTagRegistry& Registry = FGameplayTagRegistry::Get();
        for (const SGameplayTag& Owned : Tags->Tags.Tags)
        {
            Out.push_back(Registry.RequestTag(FStringView(Owned.TagName.c_str())));
        }
    }

    uint32 CGameplayTagLibrary::RequestTag(const FString& Name)
    {
        return Name.empty() ? 0u : FGameplayTagRegistry::Get().RequestTag(FStringView(Name.c_str(), Name.size()));
    }

    FString CGameplayTagLibrary::GetTagName(uint32 TagId)
    {
        return FGameplayTagRegistry::Get().GetName(TagId);
    }

    bool CGameplayTagLibrary::Matches(uint32 A, uint32 B)
    {
        return FGameplayTagRegistry::Get().Matches(A, B);
    }

    bool CGameplayTagLibrary::MatchesExact(uint32 A, uint32 B)
    {
        return FGameplayTagRegistry::Get().MatchesExact(A, B);
    }

    uint32 CGameplayTagLibrary::GetParent(uint32 TagId)
    {
        return FGameplayTagRegistry::Get().GetParent(TagId);
    }

    bool CGameplayTagLibrary::IsValidTag(uint32 TagId)
    {
        return FGameplayTagRegistry::Get().IsValid(TagId);
    }

    void CGameplayTagLibrary::AddTag(CWorld* World, ECS::FEntity Entity, uint32 TagId)
    {
        if (World == nullptr || TagId == 0)
        {
            return;
        }
        const SGameplayTag Tag = TagFromId(TagId);
        if (Tag.IsValid())
        {
            World->GetOrEmplaceComponent<SGameplayTagComponent>(Entity).Tags.AddTag(Tag);
        }
    }

    void CGameplayTagLibrary::RemoveTag(CWorld* World, ECS::FEntity Entity, uint32 TagId)
    {
        if (World == nullptr || TagId == 0)
        {
            return;
        }
        if (SGameplayTagComponent* Comp = World->TryGetComponent<SGameplayTagComponent>(Entity))
        {
            Comp->Tags.RemoveTag(TagFromId(TagId));
        }
    }

    bool CGameplayTagLibrary::HasTag(CWorld* World, ECS::FEntity Entity, uint32 TagId)
    {
        const SGameplayTagComponent* Comp = ComponentOf(World, Entity, TagId);
        if (Comp == nullptr)
        {
            return false;
        }
        FGameplayTagRegistry& Registry = FGameplayTagRegistry::Get();
        for (const SGameplayTag& Owned : Comp->Tags.Tags)
        {
            if (Registry.Matches(Registry.RequestTag(FStringView(Owned.TagName.c_str())), TagId))
            {
                return true;
            }
        }
        return false;
    }

    bool CGameplayTagLibrary::HasTagExact(CWorld* World, ECS::FEntity Entity, uint32 TagId)
    {
        const SGameplayTagComponent* Comp = ComponentOf(World, Entity, TagId);
        if (Comp == nullptr)
        {
            return false;
        }
        FGameplayTagRegistry& Registry = FGameplayTagRegistry::Get();
        for (const SGameplayTag& Owned : Comp->Tags.Tags)
        {
            if (Registry.RequestTag(FStringView(Owned.TagName.c_str())) == TagId)
            {
                return true;
            }
        }
        return false;
    }

    void CGameplayTagLibrary::ClearTags(CWorld* World, ECS::FEntity Entity)
    {
        if (World == nullptr)
        {
            return;
        }
        if (SGameplayTagComponent* Comp = World->TryGetComponent<SGameplayTagComponent>(Entity))
        {
            Comp->Tags.Tags.clear();
        }
    }
}
