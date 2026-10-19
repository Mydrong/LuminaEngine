#include "RuntimePCH.h"

#include "World/Subsystems/SpawnSubsystem.h"

#include "World/ECS/Registry.h"
#include "World/Entity/Components/LifetimeComponent.h"
#include "World/Entity/Components/ProjectileComponent.h"
#include "World/World.h"

namespace Lumina
{
    ECS::FEntity CSpawnSubsystem::SpawnProjectile(FVector3 Position, FVector3 Velocity,
        float Damage, float Lifetime, ECS::FEntity Instigator)
    {
        CWorld* World = GetWorld();
        if (World == nullptr)
        {
            return ECS::NullEntity;
        }

        FTransform SpawnTransform;
        SpawnTransform.SetLocation(Position);
        const ECS::FEntity Entity = World->ConstructEntity("Projectile", SpawnTransform);

        ECS::FRegistry& Registry = World->GetEntityRegistry();
        SProjectileComponent& Projectile = Registry.Emplace<SProjectileComponent>(Entity);
        Projectile.Velocity = Velocity;
        Projectile.Damage = Damage;
        Projectile.Instigator = Instigator;

        if (Lifetime > 0.0f)
        {
            Registry.Emplace<SLifetimeComponent>(Entity).Lifetime = Lifetime;
        }
        return Entity;
    }
}
