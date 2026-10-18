#include <gtest/gtest.h>

#include "Core/Object/ObjectCore.h"
#include "GUID/GUID.h"
#include "Input/InputModeLeases.h"
#include "Input/InputQuery.h"
#include "Input/InputViewport.h"
#include "World/World.h"

using namespace Lumina;

namespace
{
    // A world shown in its own registered viewport, so the leases have a context to write the modes into.
    struct FLeaseFixture
    {
        FInputViewportRegistry& Registry = FInputViewportRegistry::Get();
        FInputViewport Viewport;
        CWorld* World = NewObject<CWorld>(nullptr, NAME_None, FGuid::New(), OF_Transient);

        FLeaseFixture()
        {
            ECS::FRegistry& WorldRegistry = World->GetEntityRegistry();
            if (WorldRegistry.Ctx().Find<FInputModeLeases>() == nullptr)
            {
                WorldRegistry.Ctx().Emplace<FInputModeLeases>(World);
            }
            Viewport.SetWorld(World);
            Registry.Register(&Viewport);
            Input::SetInputMode(World, EInputMode::Game);
            Input::SetMouseMode(World, EMouseMode::Captured);
        }

        ~FLeaseFixture()
        {
            Registry.Unregister(&Viewport);
        }

        FInputModeLeases& Leases() const { return World->GetInputModeLeases(); }

        void ExpectModes(EInputMode InputMode, EMouseMode MouseMode) const
        {
            EXPECT_EQ(Input::GetInputMode(World), InputMode);
            EXPECT_EQ(Input::GetMouseMode(World), MouseMode);
        }
    };
}

TEST(InputModeLeases, ReleasingOneHolderKeepsTheOther)
{
    FLeaseFixture Fixture;

    const uint64 Bag = Fixture.Leases().Acquire(EInputMode::UI, EMouseMode::Normal);
    const uint64 Hud = Fixture.Leases().Acquire(EInputMode::GameAndUI, EMouseMode::Normal);
    Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);

    EXPECT_TRUE(Fixture.Leases().Release(Bag));
    Fixture.ExpectModes(EInputMode::GameAndUI, EMouseMode::Normal);

    EXPECT_TRUE(Fixture.Leases().Release(Hud));
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);
    EXPECT_EQ(Fixture.Leases().Num(), 0);
}

TEST(InputModeLeases, ReleasingTwiceOrAStrangerIsIgnored)
{
    FLeaseFixture Fixture;

    const uint64 Bag = Fixture.Leases().Acquire(EInputMode::UI, EMouseMode::Normal);
    const uint64 Map = Fixture.Leases().Acquire(EInputMode::UI, EMouseMode::Normal);
    EXPECT_TRUE(Fixture.Leases().Release(Bag));
    EXPECT_FALSE(Fixture.Leases().Release(Bag));
    EXPECT_FALSE(Fixture.Leases().Release(0));
    Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);

    EXPECT_TRUE(Fixture.Leases().Release(Map));
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);
}

TEST(InputModeLeases, LastReleaseRestoresTheModesFromBeforeTheFirst)
{
    FLeaseFixture Fixture;
    Input::SetMouseMode(Fixture.World, EMouseMode::Hidden);

    const uint64 Panel = Fixture.Leases().Acquire(EInputMode::UI, EMouseMode::Normal);
    Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);
    Fixture.Leases().Release(Panel);
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Hidden);
}

TEST(InputModeLeases, ScopedLeaseReleasesOnDestructionAndClearDropsAll)
{
    FLeaseFixture Fixture;
    {
        FScopedInputMode Scoped(Fixture.World, EInputMode::UI, EMouseMode::Normal);
        FScopedInputMode Moved = std::move(Scoped);
        EXPECT_FALSE(Scoped.IsHeld());
        EXPECT_TRUE(Moved.IsHeld());
        Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);
    }
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);

    Fixture.Leases().Acquire(EInputMode::UI, EMouseMode::Normal);
    Fixture.Leases().Acquire(EInputMode::GameAndUI, EMouseMode::Hidden);
    Fixture.Leases().Clear();
    EXPECT_EQ(Fixture.Leases().Num(), 0);
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);
}
