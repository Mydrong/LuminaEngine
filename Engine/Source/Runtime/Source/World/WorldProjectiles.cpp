#include "RuntimePCH.h"

#include "World/ECS/Registry.h"
#include "World/Entity/Components/LifetimeComponent.h"
#include "World/Entity/Components/ProjectileComponent.h"
#include "World/World.h"

namespace Lumina
{
    ECS::FEntity CWorld::SpawnProjectile(FVector3 Position, FVector3 Velocity,
        float Damage, float Lifetime, ECS::FEntity Instigator)
    {
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(Position);
        const ECS::FEntity Entity = ConstructEntity("Projectile", SpawnTransform);

        ECS::FRegistry& Registry = EntityRegistry;
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
