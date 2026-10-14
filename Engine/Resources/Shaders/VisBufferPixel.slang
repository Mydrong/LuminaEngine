#include "Includes/MeshletGeometry.slang"

[shader("fragment")]
uint FragmentMain(VIS_ID_INPUT) : SV_Target0
{
    return VisID;
}

