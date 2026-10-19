#pragma once

#include "Platform/GenericPlatform.h"
#include <cstddef>

namespace Lumina
{
    #define LE_CACHELINE_SIZE (64)
    #define LE_COUNTOF(A) ::Lumina::CountOf(A)
    #define LE_COUNTOF32(A) uint32(::Lumina::CountOf(A))
    #define LE_OFFSETOF(Type, Member) offsetof(Type, Member)
    #define LE_INCLUDES_MEMBER(Size, Type, Member) ::Lumina::IncludesMember(Size, LE_OFFSETOF(Type, Member), sizeof(decltype(Type::Member)))
    
    template<typename T, size_t N>
    constexpr size_t CountOf(const T (&)[N])
    {
        return N;
    }
    
    constexpr bool IncludesMember(size_t SizeOfStruct, size_t MemberOffset, size_t MemberSize)
    {
        return MemberOffset <= SizeOfStruct && MemberSize <= SizeOfStruct - MemberOffset;
    }

    template <typename T>
    constexpr T Align(T X, size_t Alignment)
    {
        return (T)(((size_t)X + Alignment - 1) / Alignment * Alignment);
    }
    
    template <typename T>
    T* Align(T* X, size_t Alignment)
    {
        return (T*)(((size_t)X + Alignment - 1) / Alignment * Alignment);
    }
    
    template <typename T>
    constexpr T AlignedSize(const T& Size, uint32 Alignment)
    {
        return ((Size + Alignment - 1) / Alignment) * Alignment;
    }
}
