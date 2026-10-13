#include <gtest/gtest.h>

#include "Containers/Name.h"
#include "Core/Math/Math.h"
#include "Core/Object/Cast.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectBase.h"
#include "Core/Object/ObjectCore.h"
#include "Core/Object/ScriptClass.h"
#include "Core/Reflection/Type/Function.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Scripting/ScriptFunctionMint.h"
#include "Scripting/ScriptStruct.h"
#include "Scripting/ScriptableObject.h"
#include "Scripting/ScriptableTest.h"

using namespace Lumina;

// Stands in for the managed dispatcher: reads its arguments out of the frame through the parameters
// themselves, which is the whole reason a frame is described by FProperty rather than by a signature string.
namespace
{
    int32 GThunkCalls = 0;

    void TestScriptThunk(const FFunction& Function, void* Context, void* Frame)
    {
        ++GThunkCalls;
        (void)Context;

        int32 Sum = 0;
        for (FProperty* Argument : Function.GetArguments())
        {
            if (Argument->GetType() == EPropertyTypeFlags::Int32)
            {
                Sum += *Argument->GetValuePtr<int32>(Frame);
            }
        }

        if (FProperty* Return = Function.GetReturnParam())
        {
            *Return->GetValuePtr<int32>(Frame) = Sum;
        }
    }

    Scripting::FScriptExportField ScalarField(const char* Name, EPropertyTypeFlags Kind)
    {
        Scripting::FScriptExportField Field;
        Field.Name = FName(Name);
        Field.Type = MakeShared<Scripting::FScriptExportType>();
        Field.Type->Kind = Kind;
        return Field;
    }

    Scripting::FScriptExportField EnumField(const char* Name, const char* EnumName, EPropertyTypeFlags Underlying)
    {
        Scripting::FScriptExportField Field;
        Field.Name = FName(Name);
        Field.Type = MakeShared<Scripting::FScriptExportType>();
        Field.Type->Kind = EPropertyTypeFlags::Enum;
        Field.Type->EnumName = FName(EnumName);
        Field.Type->EnumUnderlying = Underlying;
        Field.Type->EnumEntries.push_back({FName("Zero"), 0});
        Field.Type->EnumEntries.push_back({FName("One"), 1});
        return Field;
    }

    Scripting::FScriptExportField NativeStructField(const char* Name, const char* NativeType)
    {
        Scripting::FScriptExportField Field;
        Field.Name = FName(Name);
        Field.Type = MakeShared<Scripting::FScriptExportType>();
        Field.Type->Kind = EPropertyTypeFlags::Struct;
        Field.Type->NativeName = FName(NativeType);
        return Field;
    }

    struct FMinted
    {
        CScriptClass* Class = nullptr;
        FFunction*    Function = nullptr;
    };

    // The order the real mint path uses: append properties, mint functions, then link through the CDO.
    FMinted MintClassWithFunction(const char* ClassName, const Scripting::FScriptExportSchema& ParamSchema,
                                  int32 ReturnIndex)
    {
        FMinted Result;

        // Minting before the types register would link the base with no properties, and Link never runs twice.
        static const bool bReflectionReady = [] { RegisterPendingTypes(); return true; }();
        (void)bReflectionReady;

        Result.Class = FScriptableRegistry::Mint(ClassName, "CScriptableTest");
        if (Result.Class == nullptr)
        {
            return Result;
        }

        // One property, so the class gets the anchored layout record whose arena the parameters live in.
        Scripting::FScriptExportSchema Properties;
        Properties.Fields.push_back(ScalarField("Anchor", EPropertyTypeFlags::Int32));
        Scripting::AppendScriptPropertiesToClass(Result.Class, Properties);

        CScriptStruct* Record = Cast<CScriptStruct>(Result.Class->LayoutRecord.Get());
        if (Record == nullptr)
        {
            return Result;
        }

        Result.Function = Scripting::MintScriptFunction(*Result.Class, *Record, FName("Add"),
            ParamSchema, ReturnIndex, &TestScriptThunk);

        Result.Class->GetDefaultObject();
        return Result;
    }
}

TEST(ScriptFunctionMint, AScriptDeclaredFunctionBecomesAReflectedOne)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(ScalarField("A", EPropertyTypeFlags::Int32));
    Params.Fields.push_back(ScalarField("B", EPropertyTypeFlags::Int32));
    Params.Fields.push_back(ScalarField("ReturnValue", EPropertyTypeFlags::Int32));

    const FMinted Minted = MintClassWithFunction("ScriptFn_Basic", Params, 2);
    ASSERT_NE(Minted.Class, nullptr);
    ASSERT_NE(Minted.Function, nullptr);

    EXPECT_EQ(Minted.Class->FindFunction("Add"), Minted.Function) << "and is findable by name like any other";
    EXPECT_TRUE(Minted.Function->IsScriptCallable());
    ASSERT_EQ(Minted.Function->GetParams().size(), 3u);
    EXPECT_EQ(Minted.Function->GetArguments().size(), 2u);
    EXPECT_TRUE(Minted.Function->HasReturn());
}

// A parameter is not a member: it must not turn up in the class's property list.
TEST(ScriptFunctionMint, ParametersDoNotBecomeMembersOfTheClass)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(ScalarField("A", EPropertyTypeFlags::Int32));
    Params.Fields.push_back(ScalarField("ReturnValue", EPropertyTypeFlags::Int32));

    const FMinted Minted = MintClassWithFunction("ScriptFn_NotMembers", Params, 1);
    ASSERT_NE(Minted.Class, nullptr);
    ASSERT_NE(Minted.Function, nullptr);

    EXPECT_EQ(Minted.Class->GetProperty("A"), nullptr);
    EXPECT_EQ(Minted.Class->GetProperty("ReturnValue"), nullptr);
    EXPECT_NE(Minted.Class->GetProperty("Anchor"), nullptr) << "the real member is still there";
}

TEST(ScriptFunctionMint, TheFrameIsLaidOutWithoutOverlap)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(ScalarField("Small", EPropertyTypeFlags::UInt8));
    Params.Fields.push_back(ScalarField("Wide", EPropertyTypeFlags::Double));
    Params.Fields.push_back(ScalarField("ReturnValue", EPropertyTypeFlags::Int32));

    const FMinted Minted = MintClassWithFunction("ScriptFn_Layout", Params, 2);
    ASSERT_NE(Minted.Function, nullptr);

    const TSpan<FProperty* const> All = Minted.Function->GetParams();
    ASSERT_EQ(All.size(), 3u);

    for (size_t i = 1; i < All.size(); ++i)
    {
        EXPECT_GE(All[i]->Offset, All[i - 1]->Offset + All[i - 1]->GetElementSize())
            << "parameter " << i << " overlaps the one before it";
    }

    const FProperty* Last = All[All.size() - 1];
    EXPECT_LE(Last->Offset + Last->GetElementSize(), Minted.Function->GetParmsSize());

    // The wide parameter has to be aligned for its type or a load of it is misaligned.
    EXPECT_EQ(All[1]->Offset % 8u, 0u);
}

TEST(ScriptFunctionMint, CallingItReachesTheThunkThroughAFrame)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(ScalarField("A", EPropertyTypeFlags::Int32));
    Params.Fields.push_back(ScalarField("B", EPropertyTypeFlags::Int32));
    Params.Fields.push_back(ScalarField("ReturnValue", EPropertyTypeFlags::Int32));

    const FMinted Minted = MintClassWithFunction("ScriptFn_Invoke", Params, 2);
    ASSERT_NE(Minted.Function, nullptr);

    const int32 CallsBefore = GThunkCalls;

    FFunctionFrame Frame(*Minted.Function);
    Frame.At<int32>(0) = 17;
    Frame.At<int32>(1) = 25;

    // The thunk is handed the function it is dispatching, so it needs nothing from the object here.
    Frame.Invoke(Minted.Class->GetDefaultObject());

    EXPECT_EQ(GThunkCalls, CallsBefore + 1);
    EXPECT_EQ(Frame.Return<int32>(), 42);
}

// A script class can be behaviour only. The block append used to ask whether there were fields, so such a
// type got no layout record and therefore no functions either.
TEST(ScriptFunctionMint, ATypeWithFunctionsAndNoPropertiesStillGetsThem)
{
    static const bool bReflectionReady = [] { RegisterPendingTypes(); return true; }();
    (void)bReflectionReady;

    CScriptClass* Class = FScriptableRegistry::Mint("ScriptFn_BehaviourOnly", "CScriptableTest");
    ASSERT_NE(Class, nullptr);

    Scripting::FScriptExportSchema Schema;
    Schema.Functions.push_back({});
    Schema.Functions[0].Name = FName("Tick");
    Schema.Functions[0].ReturnIndex = -1;

    EXPECT_EQ(Scripting::AppendScriptPropertiesToClass(Class, Schema), 0u) << "no properties were appended";

    ASSERT_NE(Class->LayoutRecord.Get(), nullptr) << "but the record has to exist for the functions to live in";

    Class->GetDefaultObject();

    EXPECT_NE(Class->FindFunction("Tick"), nullptr);
    EXPECT_EQ(Class->GetSize(), Class->ShimSize) << "and the class stays the shim's size";
}

TEST(ScriptFunctionMint, AFunctionWithNoParametersIsStillCallable)
{
    Scripting::FScriptExportSchema Params;

    const FMinted Minted = MintClassWithFunction("ScriptFn_Void", Params, -1);
    ASSERT_NE(Minted.Class, nullptr);
    ASSERT_NE(Minted.Function, nullptr);

    EXPECT_EQ(Minted.Function->GetParams().size(), 0u);
    EXPECT_FALSE(Minted.Function->HasReturn());

    const int32 CallsBefore = GThunkCalls;
    FFunctionFrame Frame(*Minted.Function);
    Frame.Invoke(Minted.Class->GetDefaultObject());
    EXPECT_EQ(GThunkCalls, CallsBefore + 1);
}

// The managed dispatcher sizes its read and write from the slot, so a wrong width writes over a neighbor.
TEST(ScriptFunctionMint, ANarrowEnumParameterOccupiesItsUnderlyingWidth)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(EnumField("Channel", "ScriptFn_ChannelEnum", EPropertyTypeFlags::UInt8));
    Params.Fields.push_back(ScalarField("After", EPropertyTypeFlags::Int32));
    Params.Fields.push_back(ScalarField("ReturnValue", EPropertyTypeFlags::Int32));

    const FMinted Minted = MintClassWithFunction("ScriptFn_NarrowEnum", Params, 2);
    ASSERT_NE(Minted.Function, nullptr);

    const TSpan<FProperty* const> All = Minted.Function->GetParams();
    ASSERT_EQ(All.size(), 3u);

    EXPECT_EQ(All[0]->GetType(), EPropertyTypeFlags::Enum);
    EXPECT_EQ(All[0]->GetElementSize(), sizeof(uint8));
    EXPECT_GE(All[1]->Offset, All[0]->Offset + 1u);
}

TEST(ScriptFunctionMint, AWideEnumParameterOccupiesItsUnderlyingWidth)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(EnumField("Wide", "ScriptFn_WideEnum", EPropertyTypeFlags::Int64));

    const FMinted Minted = MintClassWithFunction("ScriptFn_WideEnum", Params, -1);
    ASSERT_NE(Minted.Function, nullptr);

    const TSpan<FProperty* const> All = Minted.Function->GetParams();
    ASSERT_EQ(All.size(), 1u);
    EXPECT_EQ(All[0]->GetElementSize(), sizeof(int64));
}

// The C# mirror is checked against this width, so it has to be the real sizeof, not the reflected sum.
TEST(ScriptFunctionMint, ANativeStructParameterReportsTheStructsOwnSize)
{
    Scripting::FScriptExportSchema Params;
    Params.Fields.push_back(NativeStructField("Where", "FVector3"));
    Params.Fields.push_back(ScalarField("ReturnValue", EPropertyTypeFlags::Int32));

    const FMinted Minted = MintClassWithFunction("ScriptFn_NativeStruct", Params, 1);
    ASSERT_NE(Minted.Function, nullptr);

    const CStruct* Vector = FindObject<CStruct>(FName("FVector3"));
    ASSERT_NE(Vector, nullptr);

    const TSpan<FProperty* const> All = Minted.Function->GetParams();
    ASSERT_EQ(All.size(), 2u);

    EXPECT_EQ(All[0]->GetType(), EPropertyTypeFlags::Struct);
    EXPECT_EQ(All[0]->GetElementSize(), Vector->GetAlignedSize());
    EXPECT_EQ(All[0]->GetElementSize(), sizeof(FVector3));
    EXPECT_LE(All[0]->Offset + All[0]->GetElementSize(), Minted.Function->GetParmsSize());
}
