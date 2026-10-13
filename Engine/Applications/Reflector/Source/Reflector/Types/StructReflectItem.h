#pragma once
#include <string>

namespace Lumina::Reflection
{
    class FCodeWriter;
}

namespace Lumina
{
    class IStructReflectable
    {
    public:

        virtual ~IStructReflectable() = default;

    private:

        virtual void GenerateMetadata(const std::string& InMetadata) = 0;
    };
}
