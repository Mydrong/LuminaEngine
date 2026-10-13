#pragma once

#include "ReflectedProperty.h"

namespace Lumina
{
    class FReflectedOptionalProperty : public FReflectedProperty
    {
    public:

        const char* GetTypeName() override { return nullptr; }

        std::string ElementTypeName;
    };
}
