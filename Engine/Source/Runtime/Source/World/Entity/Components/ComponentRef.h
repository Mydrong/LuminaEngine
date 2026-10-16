#pragma once

#include "Platform/GenericPlatform.h"
#include "World/ECS/Entity.h"

namespace Lumina
{
    class CWorld;
    struct FComponentOps;

    // One entity's component, found again after its pool moves. Laid out as LuminaSharp.NativeRef, so a C# field reads it in place.
    struct RUNTIME_API FComponentRef
    {
        // The component, or null when unbound or once the component is removed.
        NODISCARD void* Resolve() const;

        // Points at Entity's component of the Ops type in World, adding one first when bAdd, and stays unbound when there is none.
        void Bind(CWorld& InWorld, ECS::FEntity InEntity, const FComponentOps& InOps, bool bAdd);

        void Reset() { *this = FComponentRef(); }

        NODISCARD bool IsBound() const { return EpochSlot != nullptr; }
        NODISCARD ECS::FEntity GetEntity() const { return ECS::FEntity(Entity); }

    private:

        // Refreshed by Resolve after the pool moves, so a C# read of the same storage takes the fast path again too.
        mutable void*           Address   = nullptr;
        mutable const uint32*   EpochSlot = nullptr;
        uint64                  World     = 0;
        const FComponentOps*    Ops       = nullptr;
        mutable uint32          Epoch     = 0;
        uint32                  Entity    = 0;
        int32                   Offset    = 0;
        uint32                  Pad       = 0;
    };

    static_assert(sizeof(FComponentRef) == 48, "FComponentRef must match LuminaSharp.NativeRef field for field.");

    // A PROPERTY(Component) member is bound before OnAttach, and PROPERTY(Component = Get) only finds rather than adds.
    template<typename T>
    struct TComponentRef : FComponentRef
    {
        NODISCARD T* Get() const { return static_cast<T*>(Resolve()); }
        T* operator->() const { return Get(); }
        T& operator*() const { return *Get(); }
        explicit operator bool() const { return Resolve() != nullptr; }
    };
}
