#pragma once

#include "Core/Reflection/Type/ObjectReferenceVisitor.h"

namespace Lumina
{
    namespace ECS { class FRegistry; }

    // Every reflected component in one registry, for the object that owns it to declare, as CWorld and CPrefab do.
    RUNTIME_API void VisitRegistryObjectReferences(ECS::FRegistry& Registry, FObjectReferenceVisitor::FSlotFunc Func);
}
