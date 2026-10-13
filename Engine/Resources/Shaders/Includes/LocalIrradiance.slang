#pragma once
#include "SceneGlobals.slang"

// Irradiance from the point and spot lights the cluster grid lists at WorldPos, with no facing term.
float3 ClusteredLocalIrradiance(float3 WorldPos, float MinDistance = 0.01)
{
    const float4 Clip = WorldToClip(WorldPos);
    if (Clip.w <= 0.0)
    {
        return float3(0.0, 0.0, 0.0);
    }

    const float2 ScreenPos = ((Clip.xy / Clip.w) * 0.5 + 0.5) * float2(GetScreenSize());
    const uint   Cluster   = ComputeClusterIndex(WorldToView(WorldPos).z, ScreenPos);
    const uint   MaskWords = GetClusterMaskWords();
    const uint2  Words     = GetClusterWordRange(Cluster);
    const uint   NumLights = GetNumLights();

    float3 Irradiance = float3(0.0, 0.0, 0.0);
    for (uint Word = Words.x; Word < Words.y; ++Word)
    {
        uint Pending = GetClusterMaskWord(Cluster, Word, MaskWords);
        while (Pending != 0u)
        {
            const uint LightIndex = Word * 32u + firstbitlow(Pending);
            Pending &= Pending - 1u;
            if (LightIndex >= NumLights)
            {
                continue;
            }

            const FLight L = GetLightAt(LightIndex);
            if (HasFlag(L.Flags, ELightFlags::Directional))
            {
                continue;
            }

            const float3 ToLight  = L.Position - WorldPos;
            const float  Distance = length(ToLight);
            if (Distance > L.Radius)
            {
                continue;
            }

            float Attenuation = 1.0 / max(Distance * Distance, MinDistance * MinDistance);
            Attenuation *= 1.0 - smoothstep(clamp(L.Falloff, 0.0, 1.0), 1.0, Distance / L.Radius);

            if (HasFlag(L.Flags, ELightFlags::Spot))
            {
                const float CosAngle = dot(normalize(L.Direction), ToLight / max(Distance, 1e-4));
                Attenuation *= saturate((CosAngle - L.Angles.y) / max(L.Angles.x - L.Angles.y, 1e-3));
            }
            else if (HasFlag(L.Flags, ELightFlags::Area))
            {
                Attenuation *= saturate(dot(normalize(L.Direction), ToLight / max(Distance, 1e-4)));
            }

            Irradiance += UnpackColor(L.Color).rgb * L.Intensity * Attenuation;
        }
    }
    return Irradiance;
}
