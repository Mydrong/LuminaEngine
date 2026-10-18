#include "RuntimePCH.h"
#include "MeshLibrary.h"

#include "Assets/AssetTypes/Material/Material.h"
#include "Assets/AssetTypes/Material/MaterialInterface.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Containers/StringFormat.h"
#include "Core/Object/Package/Package.h"
#include "Renderer/Vertex.h"
#include "Tools/PrimitiveManager/PrimitiveManager.h"
#include "World/Entity/Components/DynamicMeshComponent.h"

#include <atomic>

namespace Lumina
{
    CStaticMesh* CMeshLibrary::GetPrimitiveMesh(EPrimitiveShape Shape)
    {
        CPrimitiveManager& Primitives = CPrimitiveManager::Get();
        switch (Shape)
        {
        case EPrimitiveShape::Cube:     return Primitives.CubeMesh.Get();
        case EPrimitiveShape::Sphere:   return Primitives.SphereMesh.Get();
        case EPrimitiveShape::Plane:    return Primitives.PlaneMesh.Get();
        case EPrimitiveShape::Cylinder: return Primitives.CylinderMesh.Get();
        case EPrimitiveShape::Cone:     return Primitives.ConeMesh.Get();
        case EPrimitiveShape::Capsule:  return Primitives.CapsuleMesh.Get();
        }
        return nullptr;
    }
}
