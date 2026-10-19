#include "Includes/SceneGlobals.slang"
#include "Includes/VisBufferSurface.slang"

struct FCustomStencilResolveArgs
{
    FTexture2DUInt VisBuffer;
    uint           DrawListCount;
};
FCustomStencilResolveArgs* Pass() { return GetPassArgs<FCustomStencilResolveArgs>(); }

[shader("fragment")]
uint FragmentMain(float4 SvPosition : SV_Position) : SV_Target0
{
    const uint InstanceID = ResolveVisInstance(Pass().VisBuffer.Load(int2(SvPosition.xy)), Pass().DrawListCount);
    return InstanceID == ~0u ? 0u : Instances()[InstanceID].CustomStencil;
}
