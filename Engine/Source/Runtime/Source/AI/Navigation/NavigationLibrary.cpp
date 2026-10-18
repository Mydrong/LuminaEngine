#include "RuntimePCH.h"

#include "NavigationLibrary.h"

#include "World/Entity/Systems/NavMeshSystem.h"

namespace Lumina
{
    FString CNavigationLibrary::DescribePathResult(ENavPathResult Result)
    {
        return FString(ToString(Result));
    }
}
