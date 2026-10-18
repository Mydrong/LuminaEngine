#include <gtest/gtest.h>

#include "Core/Object/ObjectCore.h"
#include "GUID/GUID.h"
#include "World/World.h"
#include "World/Entity/Components/TransformComponent.h"

using namespace Lumina;

namespace
{
    CWorld* NewWorld()
    {
        STransformComponent::StaticStruct();
        return NewObject<CWorld>(nullptr, NAME_None, FGuid::New(), OF_Transient);
    }
}

TEST(WorldApi, SetEntityTransformAddsATransformWhenMissing)
{
    CWorld* World = NewWorld();
    const ECS::FEntity Entity = World->CreateEntity();
    ASSERT_FALSE(World->HasComponent<STransformComponent>(Entity));

    const FTransform Placed(FVector3(1.0f, 2.0f, 3.0f), FVector3(0.0f), FVector3(2.0f));
    World->SetEntityTransform(Entity, Placed);

    ASSERT_TRUE(World->HasComponent<STransformComponent>(Entity));
    EXPECT_EQ(World->GetEntityTransform(Entity).GetLocation(), Placed.GetLocation());
    EXPECT_EQ(World->GetEntityScale(Entity), FVector3(2.0f));
}

TEST(WorldApi, SetEntityTransformKeepsTheExistingComponent)
{
    CWorld* World = NewWorld();
    const ECS::FEntity Entity = World->ConstructEntity("Mover");
    const STransformComponent* Before = World->TryGetComponent<STransformComponent>(Entity);

    World->SetEntityTransform(Entity, FTransform(FVector3(5.0f, 0.0f, 0.0f)));
    World->SetEntityScale(Entity, FVector3(3.0f));

    EXPECT_EQ(World->TryGetComponent<STransformComponent>(Entity), Before);
    EXPECT_EQ(World->GetEntityLocation(Entity), FVector3(5.0f, 0.0f, 0.0f));
    EXPECT_EQ(World->GetEntityScale(Entity), FVector3(3.0f));
}

TEST(WorldApi, ConstWorldReadsThroughTheSameAccessors)
{
    CWorld* World = NewWorld();
    const ECS::FEntity Entity = World->ConstructEntity("Reader");
    const CWorld& ReadOnly = *World;

    const STransformComponent& Transform = ReadOnly.GetComponent<STransformComponent>(Entity);
    const STransformComponent* Found = ReadOnly.TryGetComponent<STransformComponent>(Entity);
    static_assert(std::is_same_v<decltype(ReadOnly.GetEntityRegistry()), const ECS::FRegistry&>);

    EXPECT_EQ(&Transform, Found);
    EXPECT_FLOAT_EQ(ReadOnly.GetTimeDilation(), 1.0f);
}

TEST(WorldApi, TransformsOfAMissingEntityAreDefaults)
{
    CWorld* World = NewWorld();
    const ECS::FEntity Gone = World->CreateEntity();
    World->DestroyEntity(Gone);

    World->SetEntityTransform(Gone, FTransform(FVector3(9.0f)));
    EXPECT_EQ(World->GetEntityScale(Gone), FVector3(1.0f));
    EXPECT_EQ(World->GetEntityTransform(Gone).GetLocation(), FVector3(0.0f));
}
