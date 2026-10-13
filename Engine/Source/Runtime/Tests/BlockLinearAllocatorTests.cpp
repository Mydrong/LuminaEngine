#include <gtest/gtest.h>
#include <cstring>
#include "Memory/Allocators/Allocator.h"

using namespace Lumina;

namespace
{
    constexpr SIZE_T kBlockSize = 4 * 1024;
}

TEST(BlockLinearAllocator, ARequestLargerThanABlockSucceeds)
{
    FBlockLinearAllocator Arena(kBlockSize);

    uint8* Large = static_cast<uint8*>(Arena.Allocate(kBlockSize * 3, 64));
    ASSERT_NE(Large, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(Large) % 64, 0u);

    std::memset(Large, 0xAB, kBlockSize * 3);
    EXPECT_EQ(Large[kBlockSize * 3 - 1], 0xAB);
}

TEST(BlockLinearAllocator, ARequestJustUnderABlockStillFitsAfterAlignment)
{
    FBlockLinearAllocator Arena(kBlockSize);
    Arena.Allocate(8, 8);

    void* Tight = Arena.Allocate(kBlockSize - 64, 64);
    EXPECT_NE(Tight, nullptr);
}

TEST(BlockLinearAllocator, RewindingAMarkKeepsWhatCameBeforeIt)
{
    FBlockLinearAllocator Arena(kBlockSize);

    uint8* Before = static_cast<uint8*>(Arena.Allocate(kBlockSize * 2, 16));
    std::memset(Before, 0x11, kBlockSize * 2);

    {
        FMemMark Scope(Arena);
        uint8* Inside = Scope.AllocArray<uint8>(kBlockSize * 5);
        std::memset(Inside, 0x22, kBlockSize * 5);

        {
            FMemMark Nested(Arena);
            uint8* Deeper = Nested.AllocArray<uint8>(kBlockSize * 2);
            std::memset(Deeper, 0x33, kBlockSize * 2);
        }

        EXPECT_EQ(Inside[kBlockSize * 5 - 1], 0x22);
    }

    EXPECT_EQ(Before[0], 0x11);
    EXPECT_EQ(Before[kBlockSize * 2 - 1], 0x11);

    Arena.Reset();
    EXPECT_NE(Arena.Allocate(kBlockSize * 2, 16), nullptr);
}

TEST(BlockLinearAllocator, TheThreadScratchTakesThousandsOfBarriersAtOnce)
{
    struct FBarrierSized
    {
        uint8 Bytes[72];
    };

    FMemMark Scratch;
    FBarrierSized* Barriers = Scratch.AllocArray<FBarrierSized>(4000);
    ASSERT_NE(Barriers, nullptr);
    Barriers[3999].Bytes[71] = 7;
    EXPECT_EQ(Barriers[3999].Bytes[71], 7);
}
