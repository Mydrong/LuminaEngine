#include "Includes/SceneGlobals.slang"

// Clears the active bit of freed retained slots, so a streaming burst does not re-send their whole entries.

struct FDeactivateInstancesArgs
{
    GPUSpan<uint> Slots;        // freed slot indices
    GPUSpan<uint> CullWords;    // the retained cull entries, viewed as words
    uint Count;
    uint WordsPerEntry;
    uint FlagsWord;
    uint ClearMask;
};
FDeactivateInstancesArgs* Pass() { return GetPassArgs<FDeactivateInstancesArgs>(); }

[shader("compute")]
[numthreads(64, 1, 1)]
void ComputeMain(uint3 DTid : SV_DispatchThreadID)
{
    const uint Index = DTid.x;
    if (Index >= Pass().Count || !Pass().Slots.Contains(Index))
    {
        return;
    }

    const uint Word = Pass().Slots.Get(Index) * Pass().WordsPerEntry + Pass().FlagsWord;
    if (Pass().CullWords.Contains(Word))
    {
        Pass().CullWords.Set(Word, Pass().CullWords.Get(Word) & Pass().ClearMask);
    }
}
