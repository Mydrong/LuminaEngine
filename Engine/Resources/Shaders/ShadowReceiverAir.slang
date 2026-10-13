#include "Includes/Culling.slang"

// Raises every cascade cell a tile's view ray crosses before the tile's farthest depth, which covers its surfaces and the air fog, particles and translucency shade.

struct FPushConstants
{
    GPUSpan<uint>     Mask;
    GPUSpan<float4x4> CascadeViewProjection;
    uint              TilesX;
    uint              TilesY;
    uint              TileSize;
    uint              ScreenW;
    uint              ScreenH;
    uint              MaskDim;
    uint              NumCascades;
    // World units a tile edge strays from its center ray per unit of distance, which the marks are deepened by.
    float             TileSpreadPerDistance;
};
FPushConstants* Pass() { return GetPassArgs<FPushConstants>(); }

void RaiseCell(uint Cascade, int2 Cell, float Depth)
{
    const uint Dim = Pass().MaskDim;
    InterlockedMax(Pass().Mask.At((Cascade * Dim + (uint)Cell.y) * Dim + (uint)Cell.x)[0], asuint(saturate(Depth)));
}

// Walks the cells the cascade-space segment A to B crosses, raising each to the deeper end of its piece plus the tile's spread there.
void MarkSegment(uint Cascade, float3 A, float3 B, float WorldLength, float DepthPerWorld)
{
    const float Dim = float(Pass().MaskDim);

    // Clipped to the cascade box, since a point outside it is not shadowed by this cascade.
    float T0 = 0.0;
    float T1 = 1.0;
    const float3 D = B - A;
    [unroll]
    for (uint Axis = 0u; Axis < 3u; ++Axis)
    {
        const float Hi = Axis < 2u ? Dim : 1.0;
        if (abs(D[Axis]) < 1.0e-12)
        {
            if (A[Axis] < 0.0 || A[Axis] > Hi)
            {
                return;
            }
            continue;
        }
        const float TA = (0.0 - A[Axis]) / D[Axis];
        const float TB = (Hi - A[Axis]) / D[Axis];
        T0 = max(T0, min(TA, TB));
        T1 = min(T1, max(TA, TB));
    }
    if (T0 > T1)
    {
        return;
    }

    const float3 Start = A + D * T0;
    const float3 End   = A + D * T1;
    const float2 Dir   = End.xy - Start.xy;
    const int    Last  = int(Pass().MaskDim) - 1;

    int2 Cell = clamp(int2(floor(Start.xy)), int2(0, 0), int2(Last, Last));
    const int2 EndCell = clamp(int2(floor(End.xy)), int2(0, 0), int2(Last, Last));
    const int2 Step    = int2(Dir.x >= 0.0 ? 1 : -1, Dir.y >= 0.0 ? 1 : -1);

    // Parametric distance to the next cell boundary on each axis, and between boundaries.
    const float2 InvDir = float2(abs(Dir.x) > 1.0e-12 ? 1.0 / abs(Dir.x) : 1.0e30, abs(Dir.y) > 1.0e-12 ? 1.0 / abs(Dir.y) : 1.0e30);
    const float2 Frac   = Start.xy - float2(Cell);
    float2       TMax   = float2(Step.x > 0 ? 1.0 - Frac.x : Frac.x, Step.y > 0 ? 1.0 - Frac.y : Frac.y) * InvDir;

    // A point off the center ray sits up to the spread away, and that much nearer or farther from the sun.
    const float SpreadDepth = Pass().TileSpreadPerDistance * WorldLength * DepthPerWorld;

    float TEnter = 0.0;
    const uint MaxSteps = Pass().MaskDim * 2u + 2u;
    for (uint i = 0u; i < MaxSteps; ++i)
    {
        const float TExit  = min(min(TMax.x, TMax.y), 1.0);
        const float Global = lerp(T0, T1, TExit);
        RaiseCell(Cascade, Cell, max(lerp(Start.z, End.z, TEnter), lerp(Start.z, End.z, TExit)) + SpreadDepth * Global);
        if (all(Cell == EndCell) || TExit >= 1.0)
        {
            break;
        }
        if (TMax.x < TMax.y)
        {
            Cell.x += Step.x;
            TMax.x += InvDir.x;
        }
        else
        {
            Cell.y += Step.y;
            TMax.y += InvDir.y;
        }
        if (any(Cell < 0) || any(Cell > int2(Last, Last)))
        {
            break;
        }
        TEnter = TExit;
    }
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeMain(uint3 DTid : SV_DispatchThreadID)
{
    // One thread per tile and cascade, since a lone ray walking every cascade in turn leaves too few threads to hide the walk.
    const uint Cascade = DTid.z;
    if (DTid.x >= Pass().TilesX || DTid.y >= Pass().TilesY || Cascade >= Pass().NumCascades)
    {
        return;
    }

    // The tile's farthest opaque depth, from a pyramid that only lacks nearer late geometry and so never shortens the ray.
    const float2 Screen = float2(Pass().ScreenW, Pass().ScreenH);
    const float2 UvMin  = float2(DTid.xy * Pass().TileSize) / Screen;
    const float2 UvMax  = min(float2((DTid.xy + 1u) * Pass().TileSize) / Screen, float2(1.0, 1.0));
    const float  Level  = HiZOcclusionMip(float4(UvMin, UvMax), float2(Scene().CullData.PyramidWidth, Scene().CullData.PyramidHeight));
    const float2 UV     = (UvMin + UvMax) * 0.5;
    const float  Far    = Scene().CullData.DepthPyramid.SampleLevel(Samplers::MinReduction, UV, Level).x;

    // Sky has no depth, so the ray runs toward the far plane, which the cascade clip then trims.
    const float3 CamPos  = GetCameraPosition();
    const float3 ViewEnd = ReconstructViewPos(UV, max(Far, 1.0e-7), GetInverseCameraProjection());
    const float3 End     = mul(GetInverseCameraView(), float4(ViewEnd, 1.0)).xyz;
    const float  Length  = length(End - CamPos);

    const float4x4 VP  = Pass().CascadeViewProjection.Get(Cascade);
    const float3   NA  = mul(VP, float4(CamPos, 1.0)).xyz;
    const float3   NB  = mul(VP, float4(End, 1.0)).xyz;
    const float    Dim = float(Pass().MaskDim);
    MarkSegment(Cascade, float3((NA.xy * 0.5 + 0.5) * Dim, NA.z), float3((NB.xy * 0.5 + 0.5) * Dim, NB.z), Length, length(VP[2].xyz));
}
