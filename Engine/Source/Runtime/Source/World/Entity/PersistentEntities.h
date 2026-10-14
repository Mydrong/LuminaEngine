#pragma once

#include "Containers/Vector.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "World/ECS/Entity.h"

namespace Lumina
{
    class CEntityScript;
    class CWorld;

    // What a level change carries, lifted out of the old world before its teardown and landed in the new one after init.
    class RUNTIME_API FPersistentCarry
    {
    public:

        void Lift(CWorld* From);
        void Land(CWorld* To);

    private:

        struct FCarriedScript
        {
            TStrongObjectPtr<CEntityScript> Script;
            ECS::FEntity                    Entity;
            bool                            bUIHidden = false;
        };

        // Written the way a level is, so runtime state such as body and network ids starts over in the new world.
        TVector<uint8>          Entities;

        // The live instances, which travel by pointer so a script keeps every field and not only its saved ones.
        TVector<FCarriedScript> Scripts;
    };
}
