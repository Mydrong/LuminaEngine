#pragma once

#include "Containers/Name.h"
#include "Containers/Vector.h"
#include "Core/Object/ObjectMacros.h"
#include "GameplayTag.generated.h"

namespace Lumina
{
    // A hierarchical gameplay tag, authored as a dotted name like "Ability.Fire.Fireball".
    REFLECT()
    struct RUNTIME_API SGameplayTag
    {
        GENERATED_BODY()

        // The dotted tag name; None == an unset tag.
        PROPERTY(Editable)
        FName TagName;

        bool IsValid() const { return !TagName.IsNone(); }

        // Hierarchical, this tag IS Other or a descendant of it ("Ability.Fire.Fireball" matches "Ability.Fire").
        bool MatchesTag(const SGameplayTag& Other) const;

        // Exact (non-hierarchical) name equality.
        bool MatchesTagExact(const SGameplayTag& Other) const;

        // The immediate parent ("Ability.Fire.Fireball" -> "Ability.Fire"), or an invalid tag at the root.
        SGameplayTag GetParent() const;

        bool operator==(const SGameplayTag& Other) const { return TagName == Other.TagName; }
        bool operator!=(const SGameplayTag& Other) const { return !(*this == Other); }
    };

    // An unordered set of gameplay tags (authored as a list; each element uses the tag picker).
    REFLECT()
    struct RUNTIME_API SGameplayTagContainer
    {
        GENERATED_BODY()

        PROPERTY(Editable)
        TVector<SGameplayTag> Tags;

        void AddTag(const SGameplayTag& Tag);
        void RemoveTag(const SGameplayTag& Tag);

        // Hierarchical: any contained tag matches Tag (a "Damage.Fire" entry satisfies HasTag("Damage")).
        bool HasTag(const SGameplayTag& Tag) const;

        // Exact: a contained tag equals Tag.
        bool HasTagExact(const SGameplayTag& Tag) const;

        bool HasAny(const SGameplayTagContainer& Other) const;
        bool HasAll(const SGameplayTagContainer& Other) const;

        int32 Num() const { return (int32)Tags.size(); }
        bool IsEmpty() const { return Tags.empty(); }
    };
}
