#include "gtest/gtest.h"

#include <cmath>

#include "AI/Navigation/NavOctree.h"
#include "Config/NavigationSettings.h"
#include "NavWorldTestUtil.h"

using namespace Lumina;

namespace
{
    struct FBox
    {
        FVector3 Min;
        FVector3 Max;
    };

    bool Overlaps(const FBox& A, const FBox& B)
    {
        return A.Min.x <= B.Max.x && A.Max.x >= B.Min.x && A.Min.y <= B.Max.y && A.Max.y >= B.Min.y && A.Min.z <= B.Max.z && A.Max.z >= B.Min.z;
    }

    uint32 NextRandom(uint64& State)
    {
        State ^= State << 13;
        State ^= State >> 7;
        State ^= State << 17;
        return (uint32)(State >> 16);
    }

    float RandomRange(uint64& State, float Min, float Max)
    {
        return Min + (Max - Min) * ((float)(NextRandom(State) & 0xFFFFFF) / (float)0xFFFFFF);
    }

    FBox RandomBox(uint64& State)
    {
        const FVector3 Center(RandomRange(State, -500.0f, 500.0f), RandomRange(State, -20.0f, 20.0f), RandomRange(State, -500.0f, 500.0f));
        // Mostly small with a few huge, the way rocks sit beside a terrain tile.
        const float Size = (NextRandom(State) % 50 == 0) ? RandomRange(State, 50.0f, 400.0f) : RandomRange(State, 0.2f, 6.0f);
        return FBox{ Center - FVector3(Size), Center + FVector3(Size) };
    }

    // The octree must answer exactly what a brute force scan would, through adds, moves and removals.
    void ExpectQueriesMatch(const FNavOctree& Octree, const TVector<FBox>& Boxes, const TVector<bool>& Alive, uint64& State)
    {
        for (int32 Query = 0; Query < 200; ++Query)
        {
            const FBox Region = RandomBox(State);
            TVector<uint32> Found;
            Octree.Query(Region.Min, Region.Max, [&Found](uint32 Payload, const FVector3&, const FVector3&) { Found.push_back(Payload); });
            Algo::Sort(Found);

            TVector<uint32> Expected;
            for (uint32 Index = 0; Index < (uint32)Boxes.size(); ++Index)
            {
                if (Alive[Index] && Overlaps(Boxes[Index], Region))
                {
                    Expected.push_back(Index);
                }
            }
            ASSERT_EQ(Found, Expected);
        }
    }
}

TEST(NavOctree, QueriesMatchABruteForceScanThroughMovesAndRemovals)
{
    FNavOctree Octree;
    uint64 State = 12345;
    TVector<FBox> Boxes;
    TVector<uint32> Elements;
    TVector<bool> Alive;
    for (uint32 Index = 0; Index < 3000; ++Index)
    {
        Boxes.push_back(RandomBox(State));
        Elements.push_back(Octree.Add(Boxes.back().Min, Boxes.back().Max, Index));
        Alive.push_back(true);
    }
    EXPECT_EQ(Octree.Num(), 3000u);
    ExpectQueriesMatch(Octree, Boxes, Alive, State);

    for (uint32 Index = 0; Index < (uint32)Boxes.size(); Index += 3)
    {
        Boxes[Index] = RandomBox(State);
        Octree.Update(Elements[Index], Boxes[Index].Min, Boxes[Index].Max);
    }
    for (uint32 Index = 1; Index < (uint32)Boxes.size(); Index += 7)
    {
        Octree.Remove(Elements[Index]);
        Alive[Index] = false;
    }
    ExpectQueriesMatch(Octree, Boxes, Alive, State);
}

TEST(NavOctree, AnElementOutsideTheRootStillAnswersQueries)
{
    FNavOctree Octree(64.0f, 4.0f);
    Octree.Add(FVector3(1000.0f), FVector3(1001.0f), 7);
    int32 Hits = 0;
    Octree.Query(FVector3(999.0f), FVector3(1002.0f), [&Hits](uint32 Payload, const FVector3&, const FVector3&) { Hits += Payload == 7 ? 1 : 0; });
    EXPECT_EQ(Hits, 1);
}

// Recast voxelizes surfaces rather than volumes, so the obstacles stay lower than an agent or the ground inside them counts as walkable.
TEST(NavSourceTracking, AnObstacleMovedOntoOpenGroundRebakesBothItsTiles)
{
    NavTest::FNavWorld World;
    World.Create(64.0f);
    const ECS::FEntity Crate = NavTest::SpawnBox(World.Registry(), FVector3(-15.0f, 0.7f, 0.0f), FVector3(2.0f, 0.7f, 2.0f));
    World.Start();
    ASSERT_TRUE(World.WaitReady());

    EXPECT_FALSE(World.IsGroundWalkable(-15.0f, 0.0f));
    EXPECT_TRUE(World.IsGroundWalkable(15.0f, 0.0f));

    World.Registry().Get<STransformComponent>(Crate).SetLocation(FVector3(15.0f, 0.7f, 0.0f));
    ASSERT_TRUE(World.WaitSettled());

    EXPECT_TRUE(World.IsGroundWalkable(-15.0f, 0.0f)) << "the ground the crate left opens again";
    EXPECT_FALSE(World.IsGroundWalkable(15.0f, 0.0f)) << "the ground it now stands on is carved";
    World.Stop();
}

TEST(NavSourceTracking, AColliderAddedAfterTheBakeCarvesTheMesh)
{
    NavTest::FNavWorld World;
    World.Create(64.0f);
    World.Start();
    ASSERT_TRUE(World.WaitReady());
    EXPECT_TRUE(World.IsGroundWalkable(10.0f, 10.0f));

    NavTest::SpawnBox(World.Registry(), FVector3(10.0f, 0.7f, 10.0f), FVector3(2.0f, 0.7f, 2.0f));
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_FALSE(World.IsGroundWalkable(10.0f, 10.0f));
    World.Stop();
}

TEST(NavSourceTracking, ADestroyedColliderReopensTheGround)
{
    NavTest::FNavWorld World;
    World.Create(64.0f);
    const ECS::FEntity Wall = NavTest::SpawnBox(World.Registry(), FVector3(0.0f, 0.7f, 12.0f), FVector3(3.0f, 0.7f, 3.0f));
    World.Start();
    ASSERT_TRUE(World.WaitReady());
    EXPECT_FALSE(World.IsGroundWalkable(0.0f, 12.0f));

    World.Registry().Destroy(Wall);
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_TRUE(World.IsGroundWalkable(0.0f, 12.0f));
    World.Stop();
}

TEST(NavSourceTracking, AColliderWrittenInPlaceIsFoundByTheSweep)
{
    NavTest::FNavWorld World;
    World.Create(64.0f);
    const ECS::FEntity Pillar = NavTest::SpawnBox(World.Registry(), FVector3(-10.0f, 0.7f, -10.0f), FVector3(0.5f, 0.7f, 0.5f));
    World.Start();
    ASSERT_TRUE(World.WaitReady());
    EXPECT_TRUE(World.IsGroundWalkable(-12.5f, -10.0f));

    // A raw store raises no signal and moves nothing, which only the sweep can notice.
    World.Registry().Get<SBoxColliderComponent>(Pillar).HalfExtent = FVector3(4.0f, 0.7f, 4.0f);
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_FALSE(World.IsGroundWalkable(-12.5f, -10.0f));
    World.Stop();
}

TEST(NavSourceTracking, AnIdleWorldDirtiesNothing)
{
    NavTest::FNavWorld World;
    World.Create(64.0f);
    for (int32 i = 0; i < 40; ++i)
    {
        NavTest::SpawnBox(World.Registry(), FVector3(-28.0f + (float)(i % 8) * 7.0f, 1.0f, -28.0f + (float)(i / 8) * 7.0f), FVector3(1.0f));
    }
    World.Start();
    ASSERT_TRUE(World.WaitReady());

    for (int32 i = 0; i < 30; ++i)
    {
        World.Tick();
    }
    EXPECT_TRUE(World.Nav().Runtime.DirtyTiles.empty());
    EXPECT_TRUE(World.Nav().Runtime.SettlingTiles.empty());
    EXPECT_TRUE(World.Nav().Runtime.PendingRebakes.empty());
    World.Stop();
}

namespace
{
    // Turns the sweep off for one test, so only moves and component signals can find a change.
    struct FNoSweep
    {
        int32 Saved = GetMutableDefault<CNavigationSettings>()->SourceVerifyBudget;
        FNoSweep() { GetMutableDefault<CNavigationSettings>()->SourceVerifyBudget = 0; }
        ~FNoSweep() { GetMutableDefault<CNavigationSettings>()->SourceVerifyBudget = Saved; }
    };
}

TEST(NavSourceTracking, MovesAndSignalsAloneFindEveryChange)
{
    FNoSweep NoSweep;
    NavTest::FNavWorld World;
    World.Create(64.0f);
    const ECS::FEntity Crate = NavTest::SpawnBox(World.Registry(), FVector3(-15.0f, 0.7f, 0.0f), FVector3(2.0f, 0.7f, 2.0f));
    World.Start();
    ASSERT_TRUE(World.WaitReady());

    World.Registry().Get<STransformComponent>(Crate).SetLocation(FVector3(15.0f, 0.7f, 0.0f));
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_TRUE(World.IsGroundWalkable(-15.0f, 0.0f)) << "the move channel carried the crate";
    EXPECT_FALSE(World.IsGroundWalkable(15.0f, 0.0f));

    const ECS::FEntity Added = NavTest::SpawnBox(World.Registry(), FVector3(0.0f, 0.7f, 15.0f), FVector3(2.0f, 0.7f, 2.0f));
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_FALSE(World.IsGroundWalkable(0.0f, 15.0f)) << "the construct signal carried the new collider";

    World.Registry().Get<SBoxColliderComponent>(Added).HalfExtent = FVector3(0.5f, 0.7f, 0.5f);
    World.Registry().Patch<SBoxColliderComponent>(Added);
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_TRUE(World.IsGroundWalkable(0.0f, 17.5f)) << "the update signal carried the patched size";

    World.Registry().Destroy(Added);
    ASSERT_TRUE(World.WaitSettled());
    EXPECT_TRUE(World.IsGroundWalkable(0.0f, 15.0f)) << "the destroy signal reopened the ground";
    World.Stop();
}
