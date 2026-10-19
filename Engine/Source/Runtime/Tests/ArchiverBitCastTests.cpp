#include <gtest/gtest.h>

#include "Core/Serialization/MemoryArchiver.h"

using namespace Lumina;

namespace
{
    enum class EStoredValue : int32
    {
        Negative = -17
    };
}

TEST(ArchiverBitCast, SignedValuesAndEnumsRoundTrip)
{
    TVector<uint8> Bytes;
    int16 Small = INT16_MIN;
    int32 Medium = INT32_MIN;
    int64 Large = INT64_MIN;
    EStoredValue Enum = EStoredValue::Negative;

    {
        FMemoryWriter Writer(Bytes);
        Writer << Small << Medium << Large << Enum;
    }

    Small = 0;
    Medium = 0;
    Large = 0;
    Enum = static_cast<EStoredValue>(0);

    FMemoryReader Reader(Bytes);
    Reader << Small << Medium << Large << Enum;

    EXPECT_EQ(Small, INT16_MIN);
    EXPECT_EQ(Medium, INT32_MIN);
    EXPECT_EQ(Large, INT64_MIN);
    EXPECT_EQ(Enum, EStoredValue::Negative);
}
