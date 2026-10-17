#include "Includes/SceneGlobals.slang"
#include "Includes/VisBufferSurface.slang"

// Pixels the deferred lane does not own write 0, the clear later picker writers load and paint over.

struct FVisBufferPickerArgs
{
    FTexture2DUInt VisBuffer;
    uint           DrawListCount;
};
FVisBufferPickerArgs* Pass() { return GetPassArgs<FVisBufferPickerArgs>(); }

struct FPickerOutput
{
    uint EntityID : SV_Target0;
    // Placement reads through foliage, and grass carries its terrain's entity, so only the instance can say.
    uint Foliage  : SV_Target1;
};

[shader("fragment")]
FPickerOutput FragmentMain(float4 SvPosition : SV_Position)
{
    const uint Raw        = Pass().VisBuffer.Load(int2(SvPosition.xy));
    const uint InstanceID = ResolveVisInstance(Raw, Pass().DrawListCount);

    FPickerOutput Output;
    Output.EntityID = (InstanceID == ~0u) ? 0u : GetInstanceEntityID(InstanceID);
    Output.Foliage  = (InstanceID != ~0u && Instances()[InstanceID].HasFlag(EInstanceFlags::Foliage)) ? 1u : 0u;
    return Output;
}
