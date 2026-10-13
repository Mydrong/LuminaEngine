#pragma once
#include "SceneGlobals.slang"

// Cells of one cascade's receiver pyramid, whose levels follow each other inside the cascade's block.
uint ReceiverPyramidCells(uint Dim)
{
    return Dim * Dim + (Dim / 2u) * (Dim / 2u) + (Dim / 4u) * (Dim / 4u) + (Dim / 8u) * (Dim / 8u);
}

static const uint kReceiverPyramidLevels = 4u;

// True when the camera can see nothing deeper from the sun than the sphere, in any cell its shadow or filter could reach.
bool IsShadowUnreceived(GPUSpan<uint> Mask, uint Dim, int SunShadowDataIndex, uint CascadeIndex, float3 Center, float Radius)
{
    if (Dim == 0u || CascadeIndex >= (uint)NUM_CASCADES)
    {
        return false;
    }

    // Orthographic, so each row's length is the NDC change per world unit along that axis.
    const float4x4 CascadeVP = GetShadowViewProjection(SunShadowDataIndex, CascadeIndex);
    const float3 Ndc = mul(CascadeVP, float4(Center, 1.0)).xyz;
    const float3 R   = Radius * float3(length(CascadeVP[0].xyz), length(CascadeVP[1].xyz), length(CascadeVP[2].xyz));
    const float2 MinUV = (Ndc.xy - R.xy) * 0.5 + 0.5;
    const float2 MaxUV = (Ndc.xy + R.xy) * 0.5 + 0.5;
    if (any(MinUV < 0.0) || any(MaxUV > 1.0))
    {
        return false;
    }

    // The level where the footprint spans at most two cells per axis, as a depth pyramid picks its tap.
    const float2 Extent = (MaxUV - MinUV) * float(Dim);
    const uint   Level  = (uint)max(ceil(log2(max(max(Extent.x, Extent.y), 1.0))), 0.0);
    if (Level >= kReceiverPyramidLevels)
    {
        return false;
    }

    uint Base = CascadeIndex * ReceiverPyramidCells(Dim);
    for (uint L = 0u; L < Level; ++L)
    {
        Base += (Dim >> L) * (Dim >> L);
    }

    const uint LevelDim = Dim >> Level;
    const int2 Lo = clamp(int2(floor(MinUV * float(LevelDim))), int2(0, 0), int2(LevelDim - 1u, LevelDim - 1u));
    const int2 Hi = clamp(int2(floor(MaxUV * float(LevelDim))), int2(0, 0), int2(LevelDim - 1u, LevelDim - 1u));

    const float NearZ = Ndc.z - R.z;
    for (int y = Lo.y; y <= Hi.y; ++y)
    {
        for (int x = Lo.x; x <= Hi.x; ++x)
        {
            if (asfloat(Mask.Get(Base + (uint)y * LevelDim + (uint)x)) > NearZ)
            {
                return false;
            }
        }
    }
    return true;
}
