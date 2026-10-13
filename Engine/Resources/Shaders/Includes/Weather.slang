#pragma once
#include "SceneGlobals.slang"
#include "ShadowSampling.slang"
#include "LocalIrradiance.slang"
#include "GBuffer.slang"

bool HasWeather() { return !Scene().Weather.IsEmpty(); }

FWeatherData GetWeather() { return Scene().Weather.GetAligned(0u); }

uint WeatherHash(uint V)
{
    uint State = V * 747796405u + 2891336453u;
    uint Word  = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return (Word >> 22u) ^ Word;
}

float WeatherRandom(inout uint Seed)
{
    Seed = WeatherHash(Seed);
    return float(Seed >> 8u) * (1.0 / 16777216.0);
}

// Integer hashed so neighboring cells agree exactly on their shared corners, which the sine hash cannot promise.
float WeatherCellValue(int2 Cell)
{
    return float(WeatherHash(uint(Cell.x) * 1597334677u ^ uint(Cell.y) * 3812015801u) >> 8u) * (1.0 / 16777216.0);
}

float WeatherValueNoise(float2 P)
{
    const float2 Base = floor(P);
    const int2   Cell = int2(Base);
    float2 F = P - Base;
    F = F * F * (3.0 - 2.0 * F);
    const float A = WeatherCellValue(Cell);
    const float B = WeatherCellValue(Cell + int2(1, 0));
    const float C = WeatherCellValue(Cell + int2(0, 1));
    const float D = WeatherCellValue(Cell + int2(1, 1));
    return lerp(lerp(A, B, F.x), lerp(C, D, F.x), F.y);
}

// Three octaves normalized to [0, 1].
float PuddleHeight(float2 P)
{
    return (WeatherValueNoise(P) + 0.5 * WeatherValueNoise(P * 2.03 + 17.1) + 0.25 * WeatherValueNoise(P * 4.11 + 41.7)) / 1.75;
}

float RainExposureAt(FWeatherData W, float2 UV, float Depth, uint Width, uint Height)
{
    const float2 Texel = UV * float2(Width, Height) - 0.5;
    const float2 Frac  = frac(Texel);
    const float2 GatherUV = (floor(Texel) + 1.0) / float2(Width, Height);

    // A surface records itself in the map, so it must clear its own depth by a few centimeters to count as open.
    constexpr float kSelfBiasMeters = 0.2;
    const float Bias = kSelfBiasMeters / max(W.OcclusionDepthRange, 1.0);
    const float4 Occluder = W.RainOcclusion.GatherRed(Samplers::PointClamp, GatherUV);
    const float4 Open = select(Depth <= Occluder + Bias, float4(1.0), float4(0.0));

    // Gather order is (0,1), (1,1), (1,0), (0,0).
    const float Top    = lerp(Open.w, Open.z, Frac.x);
    const float Bottom = lerp(Open.x, Open.y, Frac.x);
    return lerp(Top, Bottom, Frac.y);
}

// Fraction of the footprint around WorldPos that rain reaches, 0 under a roof and 1 in the open; soft widens it so a roof's drip line does not stair-step.
float RainExposure(FWeatherData W, float3 WorldPos, bool bSoft = false)
{
    if (!W.RainOcclusion.IsValid())
    {
        return 1.0;
    }

    const float4 Clip = mul(W.RainViewProjection, float4(WorldPos, 1.0));
    const float3 NDC  = Clip.xyz / Clip.w;
    const float2 UV   = NDC.xy * 0.5 + 0.5;
    if (any(UV <= 0.0) || any(UV >= 1.0) || NDC.z <= 0.0)
    {
        return 1.0;
    }

    uint Width, Height;
    W.RainOcclusion.GetDimensions(Width, Height);
    if (!bSoft)
    {
        return RainExposureAt(W, UV, NDC.z, Width, Height);
    }

    const float2 Spread = 1.25 / float2(Width, Height);
    return 0.25 * (RainExposureAt(W, UV + float2(-Spread.x, -Spread.y), NDC.z, Width, Height)
                 + RainExposureAt(W, UV + float2( Spread.x, -Spread.y), NDC.z, Width, Height)
                 + RainExposureAt(W, UV + float2(-Spread.x,  Spread.y), NDC.z, Width, Height)
                 + RainExposureAt(W, UV + float2( Spread.x,  Spread.y), NDC.z, Width, Height));
}

// The large-scale shape of the ground around a point, read from the topmost surfaces the rain map recorded.
struct FRainGroundShape
{
    float3 Normal;
    // Meters the point sits below the average of its neighbors, where standing water collects first.
    float  Depression;
    bool   bValid;
};

FRainGroundShape RainGroundShape(FWeatherData W, float3 WorldPos)
{
    FRainGroundShape Shape;
    Shape.Normal     = float3(0.0, 1.0, 0.0);
    Shape.Depression = 0.0;
    Shape.bValid     = false;
    if (!W.RainOcclusion.IsValid())
    {
        return Shape;
    }

    const float4 Clip = mul(W.RainViewProjection, float4(WorldPos, 1.0));
    const float2 UV   = Clip.xy / Clip.w * 0.5 + 0.5;

    uint Width, Height;
    W.RainOcclusion.GetDimensions(Width, Height);

    // A few texels apart, so individual cobbles and pebbles average out of the ground's slope.
    constexpr float kFootprintTexels = 4.0;
    const float2 Step = kFootprintTexels / float2(Width, Height);
    if (any(UV - Step <= 0.0) || any(UV + Step >= 1.0))
    {
        return Shape;
    }

    float3 Points[5];
    const float2 Offsets[5] = { float2(0.0, 0.0), float2(-1.0, 0.0), float2(1.0, 0.0), float2(0.0, -1.0), float2(0.0, 1.0) };
    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        const float2 SampleUV = UV + Offsets[i] * Step;
        const float  Depth    = W.RainOcclusion.SampleLevel(Samplers::PointClamp, SampleUV, 0.0).r;
        // Nothing was drawn there, which is terrain or open air rather than a surface this map can describe.
        if (Depth >= 0.9999)
        {
            return Shape;
        }
        const float4 World = mul(W.RainInverseViewProjection, float4(SampleUV * 2.0 - 1.0, Depth, 1.0));
        Points[i] = World.xyz / World.w;
    }

    float3 Normal = normalize(cross(Points[4] - Points[3], Points[2] - Points[1]));
    Shape.Normal     = Normal.y < 0.0 ? -Normal : Normal;
    Shape.Depression = (Points[1].y + Points[2].y + Points[3].y + Points[4].y) * 0.25 - Points[0].y;
    Shape.bValid     = true;
    return Shape;
}

// One point of the drop field, which tiles the world so moving the camera never drags the rain along.
struct FRainDrop
{
    float3 Position;
    float3 Velocity;
    float  SizeScale;
};

FRainDrop EvaluateRainDrop(FWeatherData W, uint Index, float3 Camera)
{
    uint Seed = Index * 4u + 0x9E3779B9u;
    const float3 U         = float3(WeatherRandom(Seed), WeatherRandom(Seed), WeatherRandom(Seed));
    const float  SizeRand  = WeatherRandom(Seed);
    const float  WindRand  = WeatherRandom(Seed);
    const float  SwayPhase = WeatherRandom(Seed) * 6.2831853;

    const float Tile      = W.Motion.w;
    const float SizeScale = max(1.0 + (SizeRand * 2.0 - 1.0) * W.RainDrops.w * 0.6, 0.2);
    const float SpeedRatio = sqrt(SizeScale);
    const float Response   = (0.75 + 0.5 * WindRand) * W.RainWind.z;
    const float Time       = W.Surface2.z;

    const float  SwayFreq = 0.7 + 0.6 * WindRand;
    const float  SwayAmp  = W.RainWind.w * 0.25;
    const float2 Sway     = SwayAmp * float2(sin(Time * SwayFreq + SwayPhase), cos(Time * SwayFreq * 1.3 + SwayPhase));
    const float2 SwayVel  = SwayAmp * SwayFreq * float2(cos(Time * SwayFreq + SwayPhase), -1.3 * sin(Time * SwayFreq * 1.3 + SwayPhase));

    const float3 Offset = float3(W.RainWind.x * Response + Sway.x,
                                 -W.Motion.z * SpeedRatio,
                                 W.RainWind.y * Response + Sway.y);

    const float3 Base = U * Tile + Offset;

    FRainDrop Drop;
    Drop.Position  = Base + Tile * round((Camera - Base) / Tile);
    Drop.Velocity  = float3(W.Motion.x * Response + SwayVel.x, -W.RainFall.w * SpeedRatio, W.Motion.y * Response + SwayVel.y);
    Drop.SizeScale = SizeScale;
    return Drop;
}

// Keeps a drop beside a light finite once packed to half floats.
static const float kRainMaxLight = 30000.0;

// Light a falling drop scatters toward the eye, mostly the sky and sun it refracts.
float3 RainLightAt(float3 WorldPos)
{
    // A light's own bulb has size, so a drop brushing past it is not lit by the point singularity.
    constexpr float kLightMinDistance = 0.5;
    // A drop lenses in sky from a wide cone around it, so it reads brighter than the ambient a surface gets.
    constexpr float kRefractionGain = 2.0;
    float3 Light = GetVolumetricAmbientRadiance() * kRefractionGain + ClusteredLocalIrradiance(WorldPos, kLightMinDistance) * (0.5 * INV_PI);
    if (GetNumLights() > 0u && HasFlag(GetLightAt(0).Flags, ELightFlags::Directional))
    {
        const FLight Sun = GetLightAt(0);
        Light += UnpackColor(Sun.Color).rgb * Sun.Intensity * (0.25 * INV_PI)
               * SampleSunCascadeByContainment(Sun, WorldPos) * SampleDirectionalCloudShadow(WorldPos);
    }
    return min(Light, float3(kRainMaxLight, kRainMaxLight, kRainMaxLight));
}

// Radial slope of expanding impact rings, from a few layers of cells that each host one drop per cycle.
float2 RainRipples(float2 Position, float Time, float Rain)
{
    float2 Slope = float2(0.0, 0.0);

    [unroll]
    for (int Layer = 0; Layer < 3; ++Layer)
    {
        const float  CellsPerMeter = 2.4 + float(Layer) * 0.85;
        const float2 Q    = Position * CellsPerMeter + float2(17.3, 9.1) * float(Layer);
        const float2 Cell = floor(Q);
        const float2 F    = Q - Cell;

        uint Seed = WeatherHash(asuint(Cell.x) * 1973u ^ asuint(Cell.y) * 9277u ^ uint(Layer) * 26699u);
        const float2 Center = 0.25 + 0.5 * float2(WeatherRandom(Seed), WeatherRandom(Seed));
        const float  Phase  = WeatherRandom(Seed);
        const float  Active = WeatherRandom(Seed);
        if (Active > Rain * 0.85 + 0.15)
        {
            continue;
        }

        const float  Age    = frac(Time * 1.1 + Phase);
        const float  Radius = Age * 0.42;
        const float2 Delta  = F - Center;
        const float  Dist   = length(Delta);
        const float  X      = (Dist - Radius) * 34.0;
        const float  Wave   = abs(X) < 3.14159 ? sin(X) : 0.0;
        const float  Fade   = (1.0 - Age) * (1.0 - Age);
        Slope += (Delta / max(Dist, 1e-4)) * (Wave * Fade);
    }
    return Slope * 0.3;
}

// Darkens, glosses and floods the surface by how wet the weather has made it and how exposed it is.
void ApplyRainWetness(inout FGBufferData G, float3 WorldPos, float3 GeoNormal)
{
    if (!HasWeather() || IsUnlit(G))
    {
        return;
    }

    const FWeatherData W = GetWeather();
    const float Wetness     = W.Surface.x;
    const float PuddleLevel = W.Surface.y;
    if (Wetness <= 0.001 && PuddleLevel <= 0.001)
    {
        return;
    }

    const float Exposure = RainExposure(W, WorldPos, true);
    if (Exposure <= 0.0 && W.DebugMode == 0u)
    {
        return;
    }

    const float Up   = GeoNormal.y;
    const float Rain = W.RainDrops.x;
    const float Time = W.Surface2.z;

    // Undersides stay dry, walls take the rain but shed most of it.
    float Wet = Wetness * Exposure * saturate(Up * 0.6 + 0.55);

    const float Steep = saturate((0.75 - abs(Up)) * 2.0);
    if (Steep > 0.0 && W.Surface2.y > 0.0)
    {
        const float3 Across = normalize(cross(float3(0.0, 1.0, 0.0), GeoNormal) + float3(1e-5, 0.0, 0.0));
        uint Seed = WeatherHash(asuint(floor(dot(WorldPos, Across) * 14.0)));
        const float Column = WeatherRandom(Seed);
        const float Flow   = frac(WorldPos.y * 0.7 + Time * (0.3 + 0.6 * Column) + Column * 7.0);
        const float Rivulet = smoothstep(0.7, 1.0, Flow) * step(0.5, Column) * (0.4 + 0.6 * Rain);
        Wet = max(Wet, Wetness * Exposure * Steep * Rivulet * W.Surface2.y);
    }

    // Porous rough dielectrics drink water and darken into richer color, smooth metals only gain a film.
    const float  Porosity = saturate((G.Roughness - 0.2) * 1.6) * (1.0 - G.Metallic);
    const float3 Soaked   = G.BaseColor * lerp(0.85, 0.4, Porosity);
    const float  SoakedLuma = dot(Soaked, float3(0.2126, 0.7152, 0.0722));
    const float3 Saturated  = max(lerp(SoakedLuma.xxx, Soaked, 1.0 + 0.35 * Porosity), 0.0);
    G.BaseColor = lerp(G.BaseColor, Saturated, Wet);
    // Porous stone drinks the water rather than wearing a film, so it darkens and only smooth surfaces turn glossy.
    G.Roughness  = lerp(G.Roughness, lerp(0.1, max(G.Roughness * 0.85, 0.1), Porosity), Wet);

    float  Puddle     = 0.0;
    float  Flat       = saturate((Up - 0.9) / 0.08);
    float3 WaterUp    = float3(0.0, 1.0, 0.0);
    float  Depression = 0.0;
    if (Up > 0.6 && PuddleLevel > 0.0 && W.Surface.z > 0.0)
    {
        // Bumpy meshes tilt every triangle, so where the map knows the ground, its smoothed slope decides instead.
        const FRainGroundShape Ground = RainGroundShape(W, WorldPos);
        if (Ground.bValid)
        {
            Flat       = saturate((Ground.Normal.y - 0.97) / 0.025);
            WaterUp    = Ground.Normal;
            Depression = Ground.Depression;
        }
    }

    if (Flat > 0.0 && PuddleLevel > 0.0 && W.Surface.z > 0.0)
    {
        // Water finds the low spots first, so a few centimeters of dip counts like a much higher patch of noise.
        constexpr float kPoolingPerMeter = 12.0;
        const float2 Pattern   = WorldPos.xz / max(W.Surface.w, 0.25);
        const float  Height    = PuddleHeight(Pattern) + clamp(Depression * kPoolingPerMeter, -0.3, 0.3);
        const float  Threshold = 1.0 - W.Surface.z * PuddleLevel;
        Puddle = smoothstep(Threshold, Threshold + 0.06, Height) * Flat * Exposure;

        G.Roughness    = lerp(G.Roughness, 0.03, Puddle);
        G.Metallic     = lerp(G.Metallic, 0.0, Puddle);
        G.Specular     = lerp(G.Specular, 0.25, Puddle);
        G.WorldNormal  = normalize(lerp(G.WorldNormal, WaterUp, Puddle));
    }

    if (W.DebugMode != 0u)
    {
        const float Term = W.DebugMode == 1u ? Exposure : W.DebugMode == 2u ? Flat : W.DebugMode == 3u ? Puddle : Wet;
        G.BaseColor   = float3(Term, Term, Term);
        G.Roughness   = 1.0;
        G.Metallic    = 0.0;
        G.Emissive    = float3(Term, Term, Term) * 0.5;
        G.WorldNormal = GeoNormal;
        return;
    }

    const float RippleWeight = Rain * W.Surface2.x * Flat * Exposure * max(Puddle, Wet * 0.35);
    if (RippleWeight > 0.01)
    {
        const float2 Slope = RainRipples(WorldPos.xz, Time, Rain);
        G.WorldNormal = normalize(G.WorldNormal + float3(Slope.x, 0.0, Slope.y) * RippleWeight);
    }
}
