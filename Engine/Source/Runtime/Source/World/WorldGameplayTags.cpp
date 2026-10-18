#include "RuntimePCH.h"

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

        const SGameplayTagComponent* ComponentOf(CWorld& World, ECS::FEntity Entity, uint32 TagId)
        {
            if (TagId == 0)
            {
                return nullptr;
            }
            return World.TryGetComponent<SGameplayTagComponent>(Entity);
        }
    }
    void CWorld::GetGameplayTags(ECS::FEntity Entity, TVector<uint32>& Out)
    {
        const SGameplayTagComponent* Tags = TryGetComponent<SGameplayTagComponent>(Entity);
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

    void CWorld::AddGameplayTag(ECS::FEntity Entity, uint32 TagId)
    {
        if (TagId == 0)
        {
            return;
        }
        const SGameplayTag Tag = TagFromId(TagId);
        if (Tag.IsValid())
        {
            GetOrEmplaceComponent<SGameplayTagComponent>(Entity).Tags.AddTag(Tag);
        }
    }

    void CWorld::RemoveGameplayTag(ECS::FEntity Entity, uint32 TagId)
    {
        if (TagId == 0)
        {
            return;
        }
        if (SGameplayTagComponent* Comp = TryGetComponent<SGameplayTagComponent>(Entity))
        {
            Comp->Tags.RemoveTag(TagFromId(TagId));
        }
    }

    bool CWorld::HasGameplayTag(ECS::FEntity Entity, uint32 TagId)
    {
        const SGameplayTagComponent* Comp = ComponentOf(*this, Entity, TagId);
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

    bool CWorld::HasGameplayTagExact(ECS::FEntity Entity, uint32 TagId)
    {
        const SGameplayTagComponent* Comp = ComponentOf(*this, Entity, TagId);
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

    void CWorld::ClearGameplayTags(ECS::FEntity Entity)
    {
        if (SGameplayTagComponent* Comp = TryGetComponent<SGameplayTagComponent>(Entity))
        {
            Comp->Tags.Tags.clear();
        }
    }
}
