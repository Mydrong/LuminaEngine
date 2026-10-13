#include <gtest/gtest.h>
#include "Core/Reflection/PropertyTypeTraits.h"

#include "Core/Object/Class.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Reflection/Type/LuminaTypes.h"

using namespace Lumina;

namespace
{
    struct FMixed
    {
        int32   A = 0;
        int32   B = 0;
        int32   Unreflected = 0;
        float   C = 0.0f;
        FString Text;
    };

    CStruct* MixedStruct()
    {
        static constexpr FPropertyDesc Properties[] =
        {
            { .Name = "A", .Offset = offsetof(FMixed, A), .Type = PropertyTypeOf<int32> },
            { .Name = "B", .Offset = offsetof(FMixed, B), .Type = PropertyTypeOf<int32> },
            { .Name = "C", .Offset = offsetof(FMixed, C), .Type = PropertyTypeOf<float> },
            { .Name = "Text", .Offset = offsetof(FMixed, Text), .Type = PropertyTypeOf<FString> },
        };

        static CStruct* Struct = []
        {
            CStruct* New = NewObject<CStruct>(nullptr, "FCopyPropertiesMixed");
            New->AddToRoot();
            New->Size = sizeof(FMixed);
            InitializeAndCreateFProperties(New, Properties, (uint32)std::size(Properties));
            New->Link();
            return New;
        }();
        return Struct;
    }
}

TEST(CopyProperties, CopiesEveryReflectedMember)
{
    const FMixed Source{ .A = 1, .B = 2, .Unreflected = 99, .C = 3.5f, .Text = "copied" };
    FMixed Dest;

    MixedStruct()->CopyProperties(&Dest, &Source);

    EXPECT_EQ(Dest.A, 1);
    EXPECT_EQ(Dest.B, 2);
    EXPECT_EQ(Dest.C, 3.5f);
    EXPECT_EQ(Dest.Text, FString("copied"));
}

TEST(CopyProperties, LeavesAnUnreflectedMemberBetweenTrivialOnesAlone)
{
    const FMixed Source{ .A = 1, .B = 2, .Unreflected = 99, .C = 3.5f };
    FMixed Dest;
    Dest.Unreflected = 7;

    MixedStruct()->CopyProperties(&Dest, &Source);

    EXPECT_EQ(Dest.Unreflected, 7);
}
