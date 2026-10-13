#include "RuntimePCH.h"
#include "SceneRendererInternal.h"
#include "World/Entity/Components/WeatherComponent.h"

namespace Lumina
{
    namespace
    {
        TConsoleVar<bool> CVarWeather(
            "r.Weather",
            true,
            "Renders the weather a Weather component plays. Off skips every weather pass and surface effect.");

        TConsoleVar<int32> CVarWeatherOcclusionResolution(
            "r.Weather.OcclusionResolution",
            1024,
            "Texels across the map that keeps rain and wetness out from under roofs.");

        TConsoleVar<float> CVarWeatherOcclusionExtent(
            "r.Weather.OcclusionExtent",
            48.0f,
            "Half width in meters of the rain occlusion map around the camera.");

        TConsoleVar<int32> CVarWeatherMaxDrops(
            "r.Weather.MaxDrops",
            262144,
            "Upper bound on rain drops simulated around the camera.");

        TConsoleVar<int32> CVarWeatherSplashesPerFrame(
            "r.Weather.SplashesPerFrame",
            128,
            "Splashes spawned per frame at full rain intensity and splash density.");

        TConsoleVar<float> CVarWeatherStreakExposure(
            "r.Weather.StreakExposure",
            0.022f,
            "Seconds of fall each rain streak smears across, before the preset's streak length scale.");

        TConsoleVar<int32> CVarWeatherDebug(
            "r.Weather.Debug",
            0,
            "Paints a weather surface term over the scene. 1 rain exposure, 2 flatness, 3 puddle mask, 4 wetness.");

        constexpr float  kOcclusionHalfDepth       = 150.0f;
        constexpr uint32 kSplashCapacity           = 8192u;
        constexpr uint32 kWeatherIdleReleaseFrames = 600u;
        constexpr double kWeatherTimeWrap          = 3600.0;
        // The drop field repeats every tile, so wrapping its offsets on a whole number of tiles never moves a drop.
        constexpr double kFieldWrapTiles           = 1024.0;

        uint32 ClampedCount(float Value, uint32 Max)
        {
            if (!(Value > 0.0f))
            {
                return 0u;
            }
            return Value >= (float)Max ? Max : (uint32)Value;
        }

        struct FRainSplashGPU
        {
            FVector3 Position;
            float    SpawnTime;
            uint32   PackedNormal;
            uint32   PackedLightRG;
            uint32   PackedLightB;
            uint32   Seed;
        };
        static_assert(sizeof(FRainSplashGPU) == 32, "FRainSplashGPU must match FRainSplash in RainCommon.slang.");

        struct FRainDropsPC
        {
            RHI::TGPUSpan<FUIntVector4> Visible;
            RHI::TGPUSpan<uint32>       Args;
            uint32                      NumDrops;
            float                       Exposure;
            uint32                      _Pad0;
            uint32                      _Pad1;
        };
        static_assert(sizeof(FRainDropsPC) == 48, "FRainDropsPC must match RainDrops.slang.");

        struct FRainSplashSpawnPC
        {
            RHI::TGPUSpan<FRainSplashGPU> Splashes;
            uint32                        Cursor;
            uint32                        Count;
            uint32                        FrameSeed;
            float                         SpawnWindow;
            uint32                        GBufferB;
            uint32                        VisBuffer;
            uint32                        SlotImage;
            uint32                        _Pad0;
        };
        static_assert(sizeof(FRainSplashSpawnPC) == 48, "FRainSplashSpawnPC must match RainSplashSpawn.slang.");

        struct FRainDrawPC
        {
            RHI::TGPUSpan<FUIntVector4>   Visible;
            RHI::TGPUSpan<FRainSplashGPU> Splashes;
            float                         Exposure;
            float                         SoftFadeDistance;
            uint32                        _Pad0;
            uint32                        _Pad1;
        };
        static_assert(sizeof(FRainDrawPC) == 48, "FRainDrawPC must match FRainDrawArgs in RainCommon.slang.");

        struct FRainHazePC
        {
            uint32 HDRUAV;
            uint32 Width;
            uint32 Height;
            uint32 _Pad0;
        };
        static_assert(sizeof(FRainHazePC) == 16, "FRainHazePC must match RainHaze.slang.");

        constexpr uint32 kRainStreakVertices = 6u;
        constexpr uint32 kSplashVertices     = 30u;

        RHI::FBlendDesc PremultipliedBlend()
        {
            RHI::FBlendDesc Blend;
            Blend.bBlendEnable   = true;
            Blend.ColorOp        = RHI::EBlend::Add;
            Blend.AlphaOp        = RHI::EBlend::Add;
            Blend.SrcColorFactor = RHI::EFactor::One;
            Blend.DstColorFactor = RHI::EFactor::OneMinusSrcAlpha;
            Blend.SrcAlphaFactor = RHI::EFactor::One;
            Blend.DstAlphaFactor = RHI::EFactor::OneMinusSrcAlpha;
            return Blend;
        }
    }

    void FDefaultSceneRenderer::ExtractWeather(ECS::FRegistry& Registry, FFrameData& Frame)
    {
        Frame.Weather = {};
        if (!CVarWeather.GetValue())
        {
            return;
        }

        const ECS::TComponentStorage<SWeatherComponent> Storage = Registry.FindStorage<SWeatherComponent>();
        if (!Storage || Storage.IsEmpty())
        {
            return;
        }

        const FWeatherState* State = Registry.Ctx().Find<FWeatherState>();
        if (State == nullptr || !State->bActive)
        {
            return;
        }

        LUMINA_PROFILE_SECTION("Weather Extract");

        FFrameData::FWeather& Weather = Frame.Weather;
        Weather.bActive = true;
        Weather.State   = *State;

        const FRainParams& Rain = State->Params.Rain;
        const float Tile = Rain.Radius * 2.0f;
        if (Rain.Intensity > 0.001f)
        {
            Weather.NumDrops    = ClampedCount(Rain.DropDensity * Rain.Intensity * Tile * Tile * Tile,
                                               (uint32)Math::Clamp(CVarWeatherMaxDrops.GetValue(), 0, 1 << 22));
            Weather.NumSplashes = ClampedCount((float)CVarWeatherSplashesPerFrame.GetValue() * Rain.SplashDensity * Rain.Intensity,
                                               kSplashCapacity / 4u);
        }
        Weather.bDrops = Weather.NumDrops > 0u;

        const bool bWetSurfaces = State->Wetness > 0.001f || State->PuddleLevel > 0.001f;
        if (!Weather.bDrops && !bWetSurfaces)
        {
            return;
        }

        // The preset's steady wind rather than the gusting one, or every gust would swing the rain shadows and the wetness under them.
        const FWindParams& Wind = State->Params.Wind;
        const FVector2 SteadyWind = Wind.Direction * Wind.Speed;
        const FVector3 FallVelocity(SteadyWind.x * Rain.WindInfluence, -Math::Max(Rain.FallSpeed, 0.5f), SteadyWind.y * Rain.WindInfluence);
        const FVector3 Fall = Math::Normalize(FallVelocity);
        Weather.FallDirection = Fall;

        const uint32 Resolution = (uint32)Math::Clamp(CVarWeatherOcclusionResolution.GetValue(), 128, 4096);
        const float  Extent     = Math::Max(CVarWeatherOcclusionExtent.GetValue(), 4.0f);
        const float  TexelWorld = (Extent * 2.0f) / (float)Resolution;
        const FVector3 Up       = Math::Abs(Fall.z) < 0.99f ? FVector3(0.0f, 0.0f, 1.0f) : FVector3(1.0f, 0.0f, 0.0f);
        const FVector3 Camera   = ExtractFrame->ViewVolume.GetViewPosition();

        // Snapped to whole texels like the cascades, so the map does not crawl as the camera moves.
        const FMatrix4 Rotation = Math::LookAt(-Fall * kOcclusionHalfDepth, FVector3(0.0f), Up);
        FVector4 CenterLS = Rotation * FVector4(Camera, 1.0f);
        CenterLS.x = std::round(CenterLS.x / TexelWorld) * TexelWorld;
        CenterLS.y = std::round(CenterLS.y / TexelWorld) * TexelWorld;
        const FVector3 Center = FVector3(Math::Inverse(Rotation) * CenterLS);

        const FMatrix4 View = Math::LookAt(Center - Fall * kOcclusionHalfDepth, Center, Up);
        FMatrix4 Projection = Math::Ortho(-Extent, Extent, -Extent, Extent, 0.0f, kOcclusionHalfDepth * 2.0f);
        Projection[1][1] *= -1.0f;

        const uint32 ShadowSlot = Frame.Lighting.ShadowDataCount.fetch_add(1, std::memory_order_acquire);
        if (ShadowSlot >= (uint32)MAX_SHADOWS)
        {
            return;
        }

        const FMatrix4 ViewProjection = Projection * View;
        FLightShadowData& ShadowData = Frame.Lighting.Shadows[ShadowSlot];
        ShadowData.ViewProjection[0] = ViewProjection;
        ShadowData.Shadow[0] = {};
        ShadowData.Shadow[0].AtlasUVScale    = FVector2(1.0f);
        ShadowData.Shadow[0].ShadowMapIndex  = Constants::kIndexNone;
        ShadowData.Shadow[0].LightIndex      = Constants::kIndexNone;
        ShadowData.Shadow[0].ShadowDataIndex = (int32)ShadowSlot;

        Weather.OcclusionShadowDataIndex = (int32)ShadowSlot;
        Weather.OcclusionResolution      = Resolution;
        Weather.OcclusionViewProjection  = ViewProjection;
        Weather.OcclusionDepthRange      = kOcclusionHalfDepth * 2.0f;
        Weather.OcclusionTexelWorld      = TexelWorld;
    }

    void FDefaultSceneRenderer::ApplyWeatherToClouds(FFrameData& Frame)
    {
        Frame.Volumetrics.CloudWindOffset = FVector2(0.0f);
        if (!Frame.Weather.bActive)
        {
            return;
        }

        const FWeatherState& State = Frame.Weather.State;
        Frame.Volumetrics.CloudWindOffset = State.CloudWindOffset;

        const FCloudWeatherParams& Override = State.Params.Clouds;
        if (Override.Weight <= 0.001f)
        {
            return;
        }

        SCloudComponent Clouds = Frame.Volumetrics.bClouds ? Frame.Volumetrics.Clouds : SCloudComponent{};
        const float W = Math::Saturate(Override.Weight);
        Clouds.Coverage         = Math::Lerp(Clouds.Coverage, Override.Coverage, W);
        Clouds.Density          = Math::Lerp(Clouds.Density, Override.Density, W);
        Clouds.Billow           = Math::Lerp(Clouds.Billow, Override.Billow, W);
        Clouds.DetailStrength   = Math::Lerp(Clouds.DetailStrength, Override.DetailStrength, W);
        Clouds.SunIntensity    *= Math::Lerp(1.0f, Override.SunScale, W);
        Clouds.AmbientIntensity *= Math::Lerp(1.0f, Override.AmbientScale, W);

        Frame.Volumetrics.Clouds  = Clouds;
        Frame.Volumetrics.bClouds = Clouds.Coverage > 0.0f && Clouds.Density > 0.0f;
    }

    void FDefaultSceneRenderer::PublishWeatherGlobals(FSceneGlobalData& Globals)
    {
        const FFrameData::FWeather& Weather = RenderFrame->Weather;
        Globals.Weather = {};
        if (!Weather.bActive)
        {
            return;
        }

        WeatherIdleFrames = 0u;

        const bool bOcclusion = Weather.OcclusionCullView != Constants::kIndexNoneU32;
        if (bOcclusion && (!RainOcclusionImage.IsValid() || RainOcclusionImage.GetSizeX() != Weather.OcclusionResolution))
        {
            RetireSceneImage(RainOcclusionImage);

            RHI::FTextureDesc Desc;
            Desc.Type      = RHI::ETextureType::Tex2D;
            Desc.Dimension = FUIntVector3(Weather.OcclusionResolution, Weather.OcclusionResolution, 1);
            Desc.Format    = EFormat::D32;
            Desc.Usage     = RHI::EImageUsageFlags::DepthAttachment | RHI::EImageUsageFlags::Sampled;
            RainOcclusionImage = CreateSceneImage(Desc);
            RHI::SetDebugName(RainOcclusionImage.Texture, "Weather.RainOcclusion");
        }

        const FWeatherState& State = Weather.State;
        const FRainParams&   Rain  = State.Params.Rain;
        const double Tile = Math::Max((double)Rain.Radius * 2.0, 1.0);
        const double Wrap = Tile * kFieldWrapTiles;

        FWeatherGPU GPU;
        GPU.RainViewProjection        = Weather.OcclusionViewProjection;
        GPU.RainInverseViewProjection = Math::Inverse(Weather.OcclusionViewProjection);
        GPU.RainFall            = FVector4(Weather.FallDirection, Rain.FallSpeed);
        GPU.RainWind            = FVector4((float)std::fmod(State.RainWindX, Wrap), (float)std::fmod(State.RainWindZ, Wrap),
                                           Rain.WindInfluence, Rain.Turbulence);
        GPU.RainDrops           = FVector4(Rain.Intensity, Rain.DropDensity, Rain.DropSize, Rain.SizeVariation);
        GPU.RainLook            = FVector4(Rain.StreakLength, Rain.Brightness, Rain.Opacity, Rain.HazeDensity);
        GPU.RainTint            = FVector4(Rain.Tint, Rain.Radius);
        GPU.Surface             = FVector4(State.Wetness, State.PuddleLevel, Rain.PuddleCoverage, Rain.PuddleScale);
        GPU.Surface2            = FVector4(Rain.RippleIntensity, Rain.DripIntensity, (float)std::fmod(State.Time, kWeatherTimeWrap), Rain.SplashSize);
        GPU.Motion              = FVector4(State.Wind.x, State.Wind.y, (float)std::fmod(State.FallDistance, Wrap), (float)Tile);
        GPU.RainOcclusionIndex  = bOcclusion && RainOcclusionImage.IsValid() ? (uint32)RainOcclusionImage.GetResourceID() : Constants::kIndexNoneU32;
        GPU.OcclusionDepthRange = Weather.OcclusionDepthRange;
        GPU.OcclusionTexelWorld = Weather.OcclusionTexelWorld;
        GPU.NumDrops            = Weather.NumDrops;
        GPU.DebugMode           = (uint32)Math::Clamp(CVarWeatherDebug.GetValue(), 0, 4);

        Globals.Weather = RHI::CopyTransientArray(&GPU, 1);
    }

    void FDefaultSceneRenderer::WeatherOcclusionPass(RHI::FCmdListH CL)
    {
        const FFrameData& Frame = *RenderFrame;
        const FFrameData::FWeather& Weather = Frame.Weather;
        if (!Weather.bActive || Weather.OcclusionCullView == Constants::kIndexNoneU32 || !RainOcclusionImage.IsValid())
        {
            return;
        }

        LUMINA_PROFILE_SECTION_COLORED("Rain Occlusion Pass", tracy::Color::SteelBlue);
        SCENE_GPU_SCOPE(CL, "Rain Occlusion");

        const FUIntVector2 Extent = RainOcclusionImage.GetExtent();

        RHI::FRenderPassDesc Pass;
        Pass.DepthAttachment.Texture  = RainOcclusionImage.Texture;
        Pass.DepthAttachment.LoadOp   = RHI::ELoadOp::Clear;
        Pass.DepthAttachment.StoreOp  = RHI::EStoreOp::Store;
        Pass.DepthAttachment.Color[0] = 1.0f;
        Pass.RenderArea               = Extent;

        RHI::CmdBeginRenderPass(CL, Pass);
        SetViewportScissor(CL, Extent);

        RHI::FDepthStencilDesc DepthDesc;
        DepthDesc.DepthMode            = RHI::EDepthFlags::Read | RHI::EDepthFlags::Write;
        DepthDesc.DepthTest            = RHI::EOp::Less;
        DepthDesc.DepthBias            = 1.0f;
        DepthDesc.DepthBiasSlopeFactor = 1.5f;
        RHI::CmdSetDepthStencil(CL, DepthDesc);

        for (uint32 OpaqueIdx : Frame.Geometry.OpaqueDrawList)
        {
            const FMeshDrawCommand& Batch = Frame.Geometry.DrawCommands[OpaqueIdx];
            const bool bUseMesh = BindShadowBatchPipeline(CL, Batch, FShaderH{});
            DrawShadowBatch(CL, Batch, bUseMesh, Weather.OcclusionCullView, Weather.OcclusionShadowDataIndex, 0, Extent);
        }

        RHI::CmdEndRenderPass(CL);
        Barriers::RasterToRead(CL);
    }

    void FDefaultSceneRenderer::RainPass(RHI::FCmdListH CL)
    {
        const FFrameData& Frame = *RenderFrame;
        const FFrameData::FWeather& Weather = Frame.Weather;
        if (!Weather.bActive || !Weather.bDrops)
        {
            return;
        }

        static const FShaderH DropsShader   = FShaderLibrary::Get("RainDrops.slang");
        static const FShaderH SpawnShader   = FShaderLibrary::Get("RainSplashSpawn.slang");
        static const FShaderH StreakShader  = FShaderLibrary::Get("RainStreakVertex.slang");
        static const FShaderH SplashShader  = FShaderLibrary::Get("RainSplashVertex.slang");
        static const FShaderH PixelShader   = FShaderLibrary::Get("RainPixel.slang");
        static const FShaderH HazeShader    = FShaderLibrary::Get("RainHaze.slang");
        if (!DropsShader || !SpawnShader || !StreakShader || !SplashShader || !PixelShader || !HazeShader)
        {
            return;
        }

        LUMINA_PROFILE_SECTION_COLORED("Rain Pass", tracy::Color::SteelBlue);
        SCENE_GPU_SCOPE(CL, "Rain");

        const uint32 NumDrops = Weather.NumDrops;
        ReserveBuffer(CL, RainVisibleBuffer, (uint64)NumDrops * sizeof(FUIntVector4));
        ReserveBuffer(CL, RainArgsBuffer, sizeof(RHI::FDrawIndirectArguments));
        ReserveBuffer(CL, RainSplashBuffer, (uint64)kSplashCapacity * sizeof(FRainSplashGPU));
        if (!RainVisibleBuffer || !RainArgsBuffer || !RainSplashBuffer)
        {
            return;
        }

        const FSceneImage& HDR   = GetNamedImage(ENamedImage::HDR);
        const FSceneImage& Depth = GetNamedImage(ENamedImage::DepthAttachment);

        const FRainParams& Rain = Weather.State.Params.Rain;
        if (Rain.HazeDensity * Rain.Intensity > 0.0f && HDR.GetMipUAVIndex(0) >= 0)
        {
            FRainHazePC PC = {};
            PC.HDRUAV = (uint32)HDR.GetMipUAVIndex(0);
            PC.Width  = HDR.GetSizeX();
            PC.Height = HDR.GetSizeY();
            DispatchCompute(CL, HazeShader, PC, RenderUtils::GetGroupCount(PC.Width, 8u), RenderUtils::GetGroupCount(PC.Height, 8u));
            RHI::CmdBarrier(CL, RHI::EStageFlags::Compute, RHI::EAccessFlags::ShaderWrite,
                            RHI::EStageFlags::RasterColorOut | RHI::EStageFlags::PixelShader,
                            RHI::EAccessFlags::ColorRead | RHI::EAccessFlags::ColorWrite | RHI::EAccessFlags::ShaderRead);
        }

        const RHI::FGPURange ArgsRange{ RainArgsBuffer.Gpu, sizeof(RHI::FDrawIndirectArguments) };
        RHI::CmdMemset(CL, ArgsRange, 0u);
        Barriers::TransferToCompute(CL);

        const float Exposure = CVarWeatherStreakExposure.GetValue() * Math::Max(Rain.StreakLength, 0.0f);
        const RHI::TGPUSpan<FUIntVector4>   Visible  = RHI::TGPUSpan<FUIntVector4>::FromAddress(RainVisibleBuffer.Gpu, NumDrops);
        const RHI::TGPUSpan<FRainSplashGPU> Splashes = RHI::TGPUSpan<FRainSplashGPU>::FromAddress(RainSplashBuffer.Gpu, kSplashCapacity);

        {
            FRainDropsPC PC = {};
            PC.Visible  = Visible;
            PC.Args     = RHI::TGPUSpan<uint32>::FromAddress(RainArgsBuffer.Gpu, 4u);
            PC.NumDrops = NumDrops;
            PC.Exposure = Exposure;
            DispatchCompute(CL, DropsShader, PC, RenderUtils::GetGroupCount(NumDrops, 64u));
        }

        if (Weather.NumSplashes > 0u)
        {
            FRainSplashSpawnPC PC = {};
            PC.Splashes    = Splashes;
            PC.Cursor      = RainSplashCursor;
            PC.Count       = Weather.NumSplashes;
            PC.FrameSeed   = (uint32)(Frame.SceneGlobalData.Time * 1000.0f) * 2654435761u;
            PC.SpawnWindow = Math::Clamp(Frame.SceneGlobalData.DeltaTime, 0.0f, 0.1f);
            PC.GBufferB    = (uint32)GetNamedImage(ENamedImage::GBufferB).GetResourceID();
            PC.VisBuffer   = (uint32)GetNamedImage(ENamedImage::VisBuffer).GetResourceID();
            PC.SlotImage   = (uint32)GetNamedImage(ENamedImage::MaterialSlot).GetResourceID();
            DispatchCompute(CL, SpawnShader, PC, RenderUtils::GetGroupCount(Weather.NumSplashes, 64u));
            RainSplashCursor = (RainSplashCursor + Weather.NumSplashes) % kSplashCapacity;
        }

        RHI::CmdBarrier(CL, RHI::EStageFlags::Compute, RHI::EAccessFlags::ShaderWrite,
                        RHI::EStageFlags::VertexShader | RHI::EStageFlags::IndirectArguments,
                        RHI::EAccessFlags::ShaderRead | RHI::EAccessFlags::IndirectRead);

        RHI::FRenderAttachment Color;
        Color.Texture = HDR.Texture;
        Color.LoadOp  = RHI::ELoadOp::Load;
        Color.StoreOp = RHI::EStoreOp::Store;

        RHI::FRenderPassDesc Pass;
        Pass.ColorAttachments        = TSpan<const RHI::FRenderAttachment>(&Color, 1);
        Pass.DepthAttachment.Texture = Depth.Texture;
        Pass.DepthAttachment.LoadOp  = RHI::ELoadOp::Load;
        Pass.DepthAttachment.StoreOp = RHI::EStoreOp::Store;
        Pass.RenderArea              = HDR.GetExtent();

        RHI::CmdBeginRenderPass(CL, Pass);
        SetViewportScissor(CL, HDR.GetExtent());
        RHI::CmdSetCullMode(CL, RHI::ECullMode::None);

        RHI::FDepthStencilDesc DepthDesc;
        DepthDesc.DepthMode = RHI::EDepthFlags::Read;
        DepthDesc.DepthTest = RHI::EOp::GreaterEqual;
        RHI::CmdSetDepthStencil(CL, DepthDesc);

        FRainDrawPC DrawPC = {};
        DrawPC.Visible          = Visible;
        DrawPC.Splashes         = Splashes;
        DrawPC.Exposure         = Exposure;
        DrawPC.SoftFadeDistance = 0.1f;

        const auto DrawWith = [&](FShaderH Vertex) -> bool
        {
            FGraphicsPipelineKey Key;
            Key.VS          = Vertex;
            Key.PS          = PixelShader;
            Key.DepthFormat = EFormat::D32;
            Key.ColorTargets.push_back({ HDR.Desc.Format, PremultipliedBlend() });
            const RHI::FPipelineH Pipeline = GetOrCreatePipeline(Key);
            if (!Pipeline)
            {
                return false;
            }
            RHI::CmdSetPipeline(CL, Pipeline);
            return true;
        };

        if (DrawWith(StreakShader))
        {
            RHI::CmdDrawIndirect(CL, MakeArgs(DrawPC), ArgsRange, 1u, sizeof(RHI::FDrawIndirectArguments));
        }
        if (DrawWith(SplashShader))
        {
            RHI::CmdDraw(CL, MakeArgs(DrawPC), kSplashVertices, kSplashCapacity, 0u, 0u);
        }

        RHI::CmdEndRenderPass(CL);
        Barriers::RasterToRead(CL);
    }

    void FDefaultSceneRenderer::ReleaseIdleWeatherResources()
    {
        if (RenderFrame != nullptr && RenderFrame->Weather.bActive)
        {
            return;
        }
        if (!RainOcclusionImage.IsValid() && !RainVisibleBuffer && !RainSplashBuffer)
        {
            return;
        }
        if (++WeatherIdleFrames < kWeatherIdleReleaseFrames)
        {
            return;
        }

        RetireSceneImage(RainOcclusionImage);
        RainVisibleBuffer.Release();
        RainArgsBuffer.Release();
        RainSplashBuffer.Release();
        RainSplashCursor  = 0u;
        WeatherIdleFrames = 0u;
    }
}
