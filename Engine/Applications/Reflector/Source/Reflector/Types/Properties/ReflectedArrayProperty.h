#pragma once

#include "ReflectedProperty.h"

namespace Lumina
{
    class FReflectedArrayProperty : public FReflectedProperty
    {
    public:

        const char* GetTypeName() override { return nullptr; }

        std::string ElementTypeName;
    };
}
