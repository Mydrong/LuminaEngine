#include <gtest/gtest.h>

#include "World/ECS/Registry.h"

using namespace Lumina;

namespace
{
    struct FReserveDriver
    {
        int32 Value = 0;
    };

    // Packed, so a pool that grew mid-iteration would move every value under the caller.
    struct FReservePacked
    {
        static constexpr ECS::EComponentLayout Layout = ECS::EComponentLayout::Packed;
        int32 Value = 0;
    };

    constexpr int32 kDrivers = 300;
}

TEST(ReserveComponents, AddingOnePerViewedEntityNeverGrowsAReservedPool)
{
    ECS::FRegistry Registry;
    for (int32 Index = 0; Index < kDrivers; ++Index)
    {
        Registry.Emplace<FReserveDriver>(Registry.Create(), FReserveDriver{ Index });
    }

    // One already present, so the reserve has to count it on top of the new ones.
    const ECS::FEntity Earlier = Registry.Create();
    Registry.Emplace<FReservePacked>(Earlier, FReservePacked{ -1 });

    Registry.ReserveComponentsFor<FReservePacked, FReserveDriver>();
    const ECS::FSparseSet* Set = Registry.AssureStorage<FReservePacked>().GetSet();
    const uint8* PayloadBefore = Set->GetPackedBlock();
    const ECS::FEntity* DenseBefore = Set->GetDenseData();
    const FReservePacked* EarlierValue = &Registry.Get<FReservePacked>(Earlier);

    int32 Added = 0;
    Registry.View<FReserveDriver>().ForEach([&](ECS::FEntity Entity, FReserveDriver& Driver)
    {
        Registry.Emplace<FReservePacked>(Entity, FReservePacked{ Driver.Value });
        ++Added;
    });

    EXPECT_EQ(Added, kDrivers);
    EXPECT_EQ(Set->GetPackedBlock(), PayloadBefore);
    EXPECT_EQ(Set->GetDenseData(), DenseBefore);
    EXPECT_EQ(&Registry.Get<FReservePacked>(Earlier), EarlierValue);
    EXPECT_EQ(EarlierValue->Value, -1);
}
