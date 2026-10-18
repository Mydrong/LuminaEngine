using LuminaSharp;

namespace Lumina;

// Gameplay tags on an entity by dotted name, interned at the call.
public unsafe partial class CWorld
{
    public void AddGameplayTag(FEntity Entity, string Tag) => AddGameplayTag(Entity, CGameplayTagLibrary.RequestTag(Tag));

    public void RemoveGameplayTag(FEntity Entity, string Tag) => RemoveGameplayTag(Entity, CGameplayTagLibrary.RequestTag(Tag));

    public bool HasGameplayTag(FEntity Entity, string Tag) => HasGameplayTag(Entity, CGameplayTagLibrary.RequestTag(Tag));

    public bool HasGameplayTagExact(FEntity Entity, string Tag) => HasGameplayTagExact(Entity, CGameplayTagLibrary.RequestTag(Tag));
}
