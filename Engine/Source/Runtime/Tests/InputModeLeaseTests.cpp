#include <gtest/gtest.h>

#include "Core/Object/ObjectCore.h"
#include "GUID/GUID.h"
#include "Input/ScopedInputMode.h"
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
            Viewport.SetWorld(World);
            Registry.Register(&Viewport);
            Input::SetInputMode(World, EInputMode::Game);
            Input::SetMouseMode(World, EMouseMode::Captured);
        }

        ~FLeaseFixture()
        {
            Registry.Unregister(&Viewport);
        }

        CInputSubsystem& Leases() const { return *World->GetInput(); }

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

    const uint64 Bag = Fixture.Leases().AcquireInputMode(EInputMode::UI, EMouseMode::Normal);
    const uint64 Hud = Fixture.Leases().AcquireInputMode(EInputMode::GameAndUI, EMouseMode::Normal);
    Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);

    EXPECT_TRUE(Fixture.Leases().ReleaseInputMode(Bag));
    Fixture.ExpectModes(EInputMode::GameAndUI, EMouseMode::Normal);

    EXPECT_TRUE(Fixture.Leases().ReleaseInputMode(Hud));
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);
    EXPECT_EQ(Fixture.Leases().NumInputModeLeases(), 0);
}

TEST(InputModeLeases, ReleasingTwiceOrAStrangerIsIgnored)
{
    FLeaseFixture Fixture;

    const uint64 Bag = Fixture.Leases().AcquireInputMode(EInputMode::UI, EMouseMode::Normal);
    const uint64 Map = Fixture.Leases().AcquireInputMode(EInputMode::UI, EMouseMode::Normal);
    EXPECT_TRUE(Fixture.Leases().ReleaseInputMode(Bag));
    EXPECT_FALSE(Fixture.Leases().ReleaseInputMode(Bag));
    EXPECT_FALSE(Fixture.Leases().ReleaseInputMode(0));
    Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);

    EXPECT_TRUE(Fixture.Leases().ReleaseInputMode(Map));
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);
}

TEST(InputModeLeases, LastReleaseRestoresTheModesFromBeforeTheFirst)
{
    FLeaseFixture Fixture;
    Input::SetMouseMode(Fixture.World, EMouseMode::Hidden);

    const uint64 Panel = Fixture.Leases().AcquireInputMode(EInputMode::UI, EMouseMode::Normal);
    Fixture.ExpectModes(EInputMode::UI, EMouseMode::Normal);
    Fixture.Leases().ReleaseInputMode(Panel);
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

    Fixture.Leases().AcquireInputMode(EInputMode::UI, EMouseMode::Normal);
    Fixture.Leases().AcquireInputMode(EInputMode::GameAndUI, EMouseMode::Hidden);
    Fixture.Leases().ClearInputModes();
    EXPECT_EQ(Fixture.Leases().NumInputModeLeases(), 0);
    Fixture.ExpectModes(EInputMode::Game, EMouseMode::Captured);
}
