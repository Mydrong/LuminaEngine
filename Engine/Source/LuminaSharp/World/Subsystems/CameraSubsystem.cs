using LuminaSharp;

namespace Lumina;

public unsafe partial class CCameraSubsystem
{
    // Setting it cuts to the camera, and SetActiveCamera blends instead.
    public FEntity ActiveCamera
    {
        get => GetActiveCameraEntity();
        set => SetActiveCamera(value);
    }
}
