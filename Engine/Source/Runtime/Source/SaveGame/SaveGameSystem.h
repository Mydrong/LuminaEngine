#pragma once

#include "SaveGameEvents.h"
#include "Containers/Vector.h"
#include "Core/Object/ObjectMacros.h"
#include "World/ECS/EventQueue.h"
#include "World/Entity/Systems/EntitySystem.h"
#include "SaveGameSystem.generated.h"

namespace Lumina
{
    // Runs the queued SSaveGameRequestEvents alone at the end of a frame, where restoring can destroy and spawn safely.
    REFLECT()
    class RUNTIME_API SSaveGameSystem : public CEntitySystem
    {
        GENERATED_BODY()

    public:

        void Configure() override;
        bool HasWork(const SSystemContext& Context) override;
        void OnUpdate(const SSystemContext& Context) override;

    private:

        void Run(const SSystemContext& Context, const SSaveGameRequestEvent& Request);

        ECS::TEventReader<SSaveGameRequestEvent> Requests;
    };
}
