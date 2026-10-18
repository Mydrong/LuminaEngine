#include "RuntimePCH.h"

#include "GameplayTagLibrary.h"

#include "GameplayTags/GameplayTagComponent.h"
#include "GameplayTags/GameplayTagRegistry.h"

namespace Lumina
{
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
}
