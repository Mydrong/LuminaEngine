#pragma once

#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "MeshLibrary.generated.h"

namespace Lumina
{
    class CStaticMesh;
    class CMaterialInterface;

    REFLECT()
    enum class EPrimitiveShape : uint8
    {
        Cube,
        Sphere,
        Plane,
        Cylinder,
        Cone,
        Capsule,
    };

    REFLECT()
    class RUNTIME_API CMeshLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        // The engine's built-in primitives span one meter, so a box of size S is this mesh at scale S.
        FUNCTION()
        static CStaticMesh* GetPrimitiveMesh(EPrimitiveShape Shape);
    };
}
