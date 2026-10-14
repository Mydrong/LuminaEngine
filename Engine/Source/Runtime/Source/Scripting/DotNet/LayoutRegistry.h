#pragma once

#include "Platform/GenericPlatform.h"

// The size of every native type with a hand-written C# mirror, registered with LE_REGISTER_LAYOUT and checked by the managed LayoutValidator at boot.

namespace Lumina::DotNet
{
    RUNTIME_API void  RegisterLayout(const char* Key, int32 Size);
    RUNTIME_API int32 GetLayoutSize(const char* Name, int32 Len);   // -1 if the key is unknown

    struct FLayoutRegistrar
    {
        FLayoutRegistrar(const char* Key, int32 Size) { RegisterLayout(Key, Size); }
    };
}

#define LE_LAYOUT_CONCAT2(a, b) a##b
#define LE_LAYOUT_CONCAT(a, b)  LE_LAYOUT_CONCAT2(a, b)

// Self-register a native type's sizeof under Key (the C# [NativeLayout] key) at module load.
#define LE_REGISTER_LAYOUT(Key, Type) \
    static const ::Lumina::DotNet::FLayoutRegistrar LE_LAYOUT_CONCAT(GLayoutReg_, __COUNTER__){ Key, (int32)sizeof(Type) }
