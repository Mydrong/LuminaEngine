#include <gtest/gtest.h>
#include "Core/Object/Class.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Scripting/EntityScript.h"
#include "Scripting/ScriptableTest.h"
#include "World/World.h"
#include "World/Entity/Components/NameComponent.h"
#include "World/Entity/Components/TransformComponent.h"

using namespace Lumina;

namespace
{
    CWorld* NewRefTestWorld()
    {
        STransformComponent::StaticStruct();
        SNameComponent::StaticStruct();
        SEntityScriptComponent::StaticStruct();

        // The property chain is built when the class is finalized, which the editor does at startup.
        RegisterPendingTypes();
        CComponentRefScriptTest::StaticClass()->GetDefaultObject();

        // InitializeWorld is what registers the world with its registry, and a bare test world never runs it.
        CWorld* World = NewObject<CWorld>(nullptr, NAME_None, FGuid::New(), OF_Transient);
        ECS::GetWorldRegistry(*World).Ctx().Emplace<CWorld*>(World);
        return World;
    }

    CComponentRefScriptTest* AttachRefScript(ECS::FRegistry& Registry, ECS::FEntity Entity)
    {
        return static_cast<CComponentRefScriptTest*>(EntityScripts::Attach(Registry, Entity, CComponentRefScriptTest::StaticClass()));
    }
}

// PROPERTY(Component) adds the component when the entity lacks it, and the reference is live by OnAttach.
TEST(ComponentRef, AttachAddsAndBindsBeforeOnAttach)
{
    CWorld* World = NewRefTestWorld();
    ECS::FRegistry& Registry = ECS::GetWorldRegistry(*World);
    const ECS::FEntity Entity = Registry.Create();

    CComponentRefScriptTest* Script = AttachRefScript(Registry, Entity);
    ASSERT_NE(Script, nullptr);

    EXPECT_TRUE(Script->bTransformAtAttach);
    EXPECT_TRUE(Registry.HasAll<STransformComponent>(Entity));
    EXPECT_EQ(Script->Transform.Get(), Registry.TryGet<STransformComponent>(Entity));
    EXPECT_EQ(Script->Transform.GetEntity(), Entity);
}

// Component = Get only finds, so an entity without the component keeps it missing and the reference unbound.
TEST(ComponentRef, GetLeavesAMissingComponentAlone)
{
    CWorld* World = NewRefTestWorld();
    ECS::FRegistry& Registry = ECS::GetWorldRegistry(*World);
    const ECS::FEntity Entity = Registry.Create();

    CComponentRefScriptTest* Script = AttachRefScript(Registry, Entity);
    ASSERT_NE(Script, nullptr);

    EXPECT_FALSE(Registry.HasAll<SNameComponent>(Entity));
    EXPECT_FALSE(Script->Name.IsBound());
    EXPECT_EQ(Script->Name.Get(), nullptr);

    const ECS::FEntity Named = Registry.Create();
    Registry.Emplace<SNameComponent>(Named);
    CComponentRefScriptTest* Found = AttachRefScript(Registry, Named);
    ASSERT_NE(Found, nullptr);
    EXPECT_EQ(Found->Name.Get(), Registry.TryGet<SNameComponent>(Named));
}

// Another entity losing the component changes the pool's layout epoch, and the reference has to find its own again rather than give up.
TEST(ComponentRef, FindsTheComponentAgainAfterThePoolChanges)
{
    CWorld* World = NewRefTestWorld();
    ECS::FRegistry& Registry = ECS::GetWorldRegistry(*World);
    const ECS::FEntity Other = Registry.Create();
    Registry.Emplace<STransformComponent>(Other);
    const ECS::FEntity Entity = Registry.Create();

    CComponentRefScriptTest* Script = AttachRefScript(Registry, Entity);
    ASSERT_NE(Script, nullptr);
    Script->Transform->LocalTransform.SetLocation(FVector3(1.0f, 2.0f, 3.0f));

    Registry.Remove<STransformComponent>(Other);
    for (int32 Index = 0; Index < 4096; ++Index)
    {
        Registry.Emplace<STransformComponent>(Registry.Create());
    }

    EXPECT_EQ(Script->Transform.Get(), Registry.TryGet<STransformComponent>(Entity));
    EXPECT_EQ(Script->Transform->LocalTransform.GetLocation(), FVector3(1.0f, 2.0f, 3.0f));
}

// Removing the component leaves the reference resolving to nothing instead of the slot it used to occupy.
TEST(ComponentRef, ResolvesToNothingOnceTheComponentIsRemoved)
{
    CWorld* World = NewRefTestWorld();
    ECS::FRegistry& Registry = ECS::GetWorldRegistry(*World);
    const ECS::FEntity Entity = Registry.Create();

    CComponentRefScriptTest* Script = AttachRefScript(Registry, Entity);
    ASSERT_NE(Script, nullptr);
    ASSERT_NE(Script->Transform.Get(), nullptr);

    Registry.Remove<STransformComponent>(Entity);
    EXPECT_EQ(Script->Transform.Get(), nullptr);
    EXPECT_FALSE(static_cast<bool>(Script->Transform));
}

// A reference is found again at every attach, so it is never written into a level, an asset or an override.
TEST(ComponentRef, PropertyIsItsOwnKindAndNeverSerialized)
{
    NewRefTestWorld();
    const FProperty* Property = CComponentRefScriptTest::StaticClass()->GetProperty(FName("Transform"));
    ASSERT_NE(Property, nullptr);
    EXPECT_EQ(Property->GetType(), EPropertyTypeFlags::ComponentRef);
    EXPECT_FALSE(Property->ShouldSerialize());
    EXPECT_EQ(Property->GetElementSize(), sizeof(FComponentRef));
}
