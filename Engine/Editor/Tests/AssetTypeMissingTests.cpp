#include <gtest/gtest.h>

#include "Assets/AssetRegistry/AssetData.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Core/Object/Class.h"

using namespace Lumina;

TEST(AssetTypeMissing, OnlyAnUnregisteredClassCountsAsMissing)
{
    CStaticMesh::StaticClass();
    RegisterPendingTypes();

    FAssetData Data;
    EXPECT_FALSE(Data.IsClassMissing());

    Data.AssetClass = CStaticMesh::StaticClass()->GetName();
    EXPECT_FALSE(Data.IsClassMissing());

    Data.AssetClass = FName("CRemovedAssetTypeForTest");
    EXPECT_TRUE(Data.IsClassMissing());
}
