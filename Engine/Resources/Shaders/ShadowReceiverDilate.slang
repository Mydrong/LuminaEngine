#include "Includes/SceneGlobals.slang"
#include "Includes/GlobalRHI.slang"

// Level 0 spreads each receiver cell over a radius covering an air tile's ray fan and the shadow filter, and coarser levels take the 2x2 maximum.

struct FPushConstants
{
    GPUSpan<uint> Source;
    GPUSpan<uint> Dest;
    // Cells per side of Dest, and of Source whose side is twice that unless this is the dilation.
    uint          DestDim;
    uint          NumCascades;
    uint          bDilate;
    // Distance between consecutive cascades' blocks in each span.
    uint          SourceStride;
    uint          DestStride;
    uint          _Pad0;
    uint          _Pad1;
    uint          _Pad2;
};
FPushConstants* Pass() { return GetPassArgs<FPushConstants>(); }

static const int kDilateRadius = 2;

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeMain(uint3 DTid : SV_DispatchThreadID)
{
    const int Dim = int(Pass().DestDim);
    if (DTid.x >= (uint)Dim || DTid.y >= (uint)Dim || DTid.z >= Pass().NumCascades)
    {
        return;
    }

    uint Deepest = 0u;
    if (Pass().bDilate != 0u)
    {
        const uint Base = DTid.z * Pass().SourceStride;
        for (int y = max(int(DTid.y) - kDilateRadius, 0); y <= min(int(DTid.y) + kDilateRadius, Dim - 1); ++y)
        {
            for (int x = max(int(DTid.x) - kDilateRadius, 0); x <= min(int(DTid.x) + kDilateRadius, Dim - 1); ++x)
            {
                Deepest = max(Deepest, Pass().Source.Get(Base + (uint)(y * Dim + x)));
            }
        }
    }
    else
    {
        const uint SrcDim = (uint)Dim * 2u;
        const uint Base   = DTid.z * Pass().SourceStride;
        const uint2 Src   = DTid.xy * 2u;
        Deepest = max(max(Pass().Source.Get(Base + Src.y * SrcDim + Src.x), Pass().Source.Get(Base + Src.y * SrcDim + Src.x + 1u)),
                      max(Pass().Source.Get(Base + (Src.y + 1u) * SrcDim + Src.x), Pass().Source.Get(Base + (Src.y + 1u) * SrcDim + Src.x + 1u)));
    }

    // Non-negative floats order like their bits, so the maximum needs no conversion.
    Pass().Dest.Set(DTid.z * Pass().DestStride + DTid.y * (uint)Dim + DTid.x, Deepest);
}
