#include "RuntimePCH.h"

#include "World/ECS/Registry.h"
#include "World/Entity/Components/ParticleSystemComponent.h"
#include "World/World.h"

namespace Lumina
{
    static float ResolveLifetime(const CParticleSystem* ParticleSystem, float Lifetime)
    {
        return Lifetime < 0.0f ? ParticleSystem->GetOneShotLength() : Lifetime;
    }

    ECS::FEntity CWorld::SpawnParticleSystem(CParticleSystem* ParticleSystem,
        const FTransform& SpawnTransform, float Lifetime)
    {
        if (ParticleSystem == nullptr)
        {
            return ECS::NullEntity;
        }

        const ECS::FEntity Spawned = ConstructEntity("ParticleEffect", SpawnTransform);
        SParticleSystemComponent& Effect = EmplaceComponent<SParticleSystemComponent>(Spawned);
        Effect.ParticleSystem = ParticleSystem;
        Effect.bBurstOnSpawn = true;
        Effect.Activate(true);

        SetEntityLifetime(Spawned, ResolveLifetime(ParticleSystem, Lifetime));
        return Spawned;
    }

    ECS::FEntity CWorld::SpawnParticleSystemAttached(CParticleSystem* ParticleSystem, ECS::FEntity Parent, const FName& Socket, FVector3 Offset, float Lifetime)
    {
        if (ParticleSystem == nullptr || !IsValidEntity(Parent))
        {
            return ECS::NullEntity;
        }

        // Attaching snaps the child onto the socket, so spawning at a world point first would be undone.
        const ECS::FEntity Spawned = ConstructEntity("ParticleEffect", FTransform());
        SParticleSystemComponent& Effect = EmplaceComponent<SParticleSystemComponent>(Spawned);
        Effect.ParticleSystem = ParticleSystem;
        Effect.EmitterOffset = Offset;
        Effect.bBurstOnSpawn = true;
        Effect.Activate(true);

        // A managed caller spells "no socket" as an empty string, which does not intern to NAME_None.
        if (Socket.IsNone() || FStringView(Socket.c_str()).empty())
        {
            SetParent(Spawned, Parent);
        }
        else
        {
            AttachEntityToSocket(Spawned, Parent, Socket);
        }

        SetEntityLifetime(Spawned, ResolveLifetime(ParticleSystem, Lifetime));
        return Spawned;
    }

    void CWorld::GetParticleCollisions(ECS::FEntity Entity, int32 EmitterIndex, TVector<SParticleCollision>& OutCollisions)
    {
        const SParticleSystemComponent* Effect = TryGetComponent<SParticleSystemComponent>(Entity);
        if (Effect == nullptr)
        {
            return;
        }

        for (const SParticleCollision& Hit : Effect->Collisions)
        {
            if (EmitterIndex < 0 || Hit.EmitterIndex == EmitterIndex)
            {
                OutCollisions.push_back(Hit);
            }
        }
    }
}
