#include "Includes/RainCommon.slang"

[shader("pixel")]
float4 PixelMain(FRainVertexOutput In) : SV_Target0
{
    float Shape;
    if (In.Kind == kRainKindRing)
    {
        const float2 Centered = In.Coord * 2.0 - 1.0;
        const float  Radius   = length(Centered);
        Shape = saturate(1.0 - abs(Radius - 0.8) * 6.0) * step(Radius, 1.0);
    }
    else
    {
        const float Across = In.Coord.x;
        const float Along  = In.Coord.y;
        Shape = exp(-Across * Across * 3.0) * smoothstep(0.0, 0.12, Along) * (1.0 - 0.6 * smoothstep(0.6, 1.0, Along));
    }

    float Soft = 1.0;
    const FTexture2D SceneDepth = Scene().SceneDepth;
    if (SceneDepth.IsValid() && RainArgs().SoftFadeDistance > 0.0)
    {
        const float SceneRaw = SceneDepth.Load(int3(int2(In.Position.xy), 0)).r;
        Soft = saturate((LinearizeSceneDepth(SceneRaw) - In.ViewDepth) / RainArgs().SoftFadeDistance);
    }

    const float Alpha = saturate(In.Alpha * Shape * Soft);
    return float4(In.Color * Alpha, Alpha);
}
