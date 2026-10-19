#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>

#include "Containers/Name.h"

using namespace Lumina;

namespace
{
    constexpr int kIterations = 500000;
    constexpr int kRepeats = 5;

    template <typename F>
    double BestNanos(F&& Construct)
    {
        double Best = 1e30;
        for (int Repeat = 0; Repeat < kRepeats; ++Repeat)
        {
            uint64 Sum = 0;
            const auto Start = std::chrono::steady_clock::now();
            for (int Index = 0; Index < kIterations; ++Index)
            {
                Sum += Construct().GetID();
            }
            const auto End = std::chrono::steady_clock::now();
            const double Nanos = std::chrono::duration<double, std::nano>(End - Start).count() / kIterations;
            Best = Math::Min(Best, Nanos);
            EXPECT_NE(Sum, 0u);
        }
        return Best;
    }
}

TEST(FNameBench, CStringHit)
{
    FName Existing("NameLookupBenchmark");
    const double Nanos = BestNanos([] { return FName("NameLookupBenchmark"); });
    std::printf("FName CString hit %.2f ns\n", Nanos);
}

TEST(FNameBench, CStringCaseFoldedHit)
{
    FName Existing("NameLookupBenchmark");
    const double Nanos = BestNanos([] { return FName("NAMELOOKUPBENCHMARK"); });
    std::printf("FName CString case folded hit %.2f ns\n", Nanos);
}

TEST(FNameBench, StringViewHit)
{
    FName Existing("NameLookupBenchmark");
    const FStringView Text("NameLookupBenchmark");
    const double Nanos = BestNanos([&] { return FName(Text); });
    std::printf("FName string view hit %.2f ns\n", Nanos);
}
