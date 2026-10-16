#pragma once

#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Core/Object/Object.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/Object/ObjectMacros.h"
#include "PrimitiveManager.generated.h"

namespace Lumina
{
    REFLECT()
    class RUNTIME_API CPrimitiveManager : public CObject
    {
        GENERATED_BODY()
    public:

        CPrimitiveManager();

        void Initialize();

        static CPrimitiveManager& Get();

        /** Unit cube mesh. */
        PROPERTY(Hidden, NoSerialize)
        TStrongObjectPtr<CStaticMesh> CubeMesh;

        /** Unit sphere mesh. */
        PROPERTY(Hidden, NoSerialize)
        TStrongObjectPtr<CStaticMesh> SphereMesh;

        /** Unit plane mesh. */
        PROPERTY(Hidden, NoSerialize)
        TStrongObjectPtr<CStaticMesh> PlaneMesh;

        /** Unit cylinder mesh. */
        PROPERTY(Hidden, NoSerialize)
        TStrongObjectPtr<CStaticMesh> CylinderMesh;

        /** Unit cone mesh. */
        PROPERTY(Hidden, NoSerialize)
        TStrongObjectPtr<CStaticMesh> ConeMesh;

        /** Unit capsule mesh. */
        PROPERTY(Hidden, NoSerialize)
        TStrongObjectPtr<CStaticMesh> CapsuleMesh;
    };
}
