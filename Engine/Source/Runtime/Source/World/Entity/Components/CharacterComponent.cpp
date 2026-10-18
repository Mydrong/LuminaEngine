#include "RuntimePCH.h"
#include "CharacterComponent.h"

// Included only so the handle is a complete type where these special members are defined.
#include "Physics/API/Box3D/Box3DCharacterHandle.h"

namespace Lumina
{
    SCharacterPhysicsComponent::SCharacterPhysicsComponent() = default;

    void SCharacterPhysicsComponent::GetActiveCapsule(const SCharacterMovementComponent* Movement, float& OutHalfHeight, float& OutCenterDrop) const
    {
        // Crouching shrinks from the top, so the center drops by the height lost and the feet stay put.
        const bool bCrouched = Movement != nullptr && Movement->bIsCrouched;
        OutHalfHeight = bCrouched ? Math::Min(Movement->CrouchHalfHeight, HalfHeight) : HalfHeight;
        OutCenterDrop = HalfHeight - OutHalfHeight;
    }
    SCharacterPhysicsComponent::~SCharacterPhysicsComponent() = default;
    SCharacterPhysicsComponent::SCharacterPhysicsComponent(const SCharacterPhysicsComponent&) = default;
    SCharacterPhysicsComponent& SCharacterPhysicsComponent::operator=(const SCharacterPhysicsComponent&) = default;
    SCharacterPhysicsComponent::SCharacterPhysicsComponent(SCharacterPhysicsComponent&&) noexcept = default;
    SCharacterPhysicsComponent& SCharacterPhysicsComponent::operator=(SCharacterPhysicsComponent&&) noexcept = default;

}
