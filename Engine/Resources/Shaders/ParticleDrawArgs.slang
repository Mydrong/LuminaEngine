#include "Includes/SceneGlobals.slang"

// Lays every particle draw's arguments out back to back, so consecutive emitters on one pipeline become one multi-draw.

struct FParticleDrawArgsSource
{
    // The sort's compacted draw, or empty to draw VertexCount vertices as planned.
    GPUSpan<FDrawIndirectArguments> SortedArgs;
    uint                            VertexCount;
    uint                            _Pad0;
    uint                            _Pad1;
    uint                            _Pad2;
};

struct FParticleDrawArgsPass
{
    GPUSpan<FParticleDrawArgsSource> Sources;
    GPUSpan<FDrawIndirectArguments>  OutArgs;
};
FParticleDrawArgsPass* Pass() { return GetPassArgs<FParticleDrawArgsPass>(); }

[numthreads(64, 1, 1)]
void ComputeMain(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint Index = DispatchThreadID.x;
    if (Index >= Pass().Sources.Count)
    {
        return;
    }

    const FParticleDrawArgsSource Source = Pass().Sources.Get(Index);
    FDrawIndirectArguments Args;
    if (Source.SortedArgs.Count != 0u)
    {
        Args = Source.SortedArgs.Get(0u);
    }
    else
    {
        Args.VertexCount   = Source.VertexCount;
        Args.InstanceCount = 1u;
        Args.VertexOffset  = 0u;
        Args.FirstInstance = 0u;
    }
    Pass().OutArgs.Set(Index, Args);
}
