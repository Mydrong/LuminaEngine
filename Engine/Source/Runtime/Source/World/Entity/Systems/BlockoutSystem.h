#pragma once

#include "EntitySystem.h"
#include "Core/Object/ObjectMacros.h"
#include "BlockoutSystem.generated.h"

namespace Lumina
{
    struct SBlockoutComponent;
    struct SDynamicMeshComponent;

    /** Meshes every SBlockoutComponent whose parameters moved, and every one a load left unbuilt. */
    REFLECT()
    class RUNTIME_API SBlockoutSystem : public CEntitySystem
    {
        GENERATED_BODY()
    public:

        void Configure() override;
        void OnUpdate(const SSystemContext& Context) override;

        /** Immediate rebuild for callers that cannot wait a tick, such as the editor placing a shape. */
        static bool Rebuild(SBlockoutComponent& Shape, SDynamicMeshComponent& Mesh, bool bKeepCPUData);
    };
}
