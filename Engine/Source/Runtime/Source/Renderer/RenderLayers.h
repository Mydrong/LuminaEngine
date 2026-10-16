#pragma once

#include "Containers/Name.h"
#include "Core/Object/ObjectMacros.h"
#include "Shared/SharedConstants.h"

#include "RenderLayers.generated.h"

namespace Lumina
{
    namespace RenderLayers
    {
        constexpr uint32 kCount          = 32;
        constexpr uint32 kForegroundBit  = 31;
        constexpr uint32 kDefaultMask    = RENDER_LAYER_DEFAULT_MASK;
        constexpr uint32 kForegroundMask = RENDER_LAYER_FOREGROUND_MASK;
        constexpr uint32 kAllMask        = 0xFFFFFFFFu;

        static_assert(kForegroundMask == 1u << kForegroundBit, "SharedConstants.h and the bit index disagree.");
    }

    // Which of the 32 render layers something belongs to, or for a camera, which layers it draws. Names come from CRenderLayerSettings.
    REFLECT()
    struct RUNTIME_API SRenderLayerMask
    {
        GENERATED_BODY()

        SRenderLayerMask() = default;
        constexpr explicit SRenderLayerMask(uint32 InBits) : Bits(InBits) {}

        PROPERTY(Editable)
        uint32 Bits = RenderLayers::kDefaultMask;

        FUNCTION()
        bool HasLayer(int32 Layer) const { return IsValidLayer(Layer) && (Bits & (1u << Layer)) != 0u; }

        FUNCTION()
        void SetLayer(int32 Layer, bool bEnabled)
        {
            if (IsValidLayer(Layer))
            {
                Bits = bEnabled ? (Bits | (1u << Layer)) : (Bits & ~(1u << Layer));
            }
        }

        FUNCTION()
        bool HasLayerNamed(FName Name) const { return HasLayer(FindLayer(Name)); }

        // A name no layer carries leaves the mask unchanged.
        FUNCTION()
        void SetLayerNamed(FName Name, bool bEnabled) { SetLayer(FindLayer(Name), bEnabled); }

        FUNCTION()
        bool Intersects(SRenderLayerMask Other) const { return (Bits & Other.Bits) != 0u; }

        // None for a layer nobody named.
        FUNCTION()
        static FName GetLayerName(int32 Layer);

        // The layer carrying Name, or -1 when no layer does.
        FUNCTION()
        static int32 FindLayer(FName Name);

        static bool IsValidLayer(int32 Layer) { return Layer >= 0 && Layer < (int32)RenderLayers::kCount; }
    };

    // What the active camera asks of the layers, handed to the render scene alongside its view.
    struct FViewLayerSettings
    {
        uint32 CullMask             = RenderLayers::kAllMask;
        // Zero draws the Foreground layer in place like any other layer.
        float  ForegroundFOV        = 0.0f;
        float  ForegroundDepthScale = 1.0f;

        bool Draws(const SRenderLayerMask& Layers) const { return Layers.Intersects(SRenderLayerMask(CullMask)); }
        bool IsForegroundActive() const { return ForegroundFOV > 0.0f && (CullMask & RenderLayers::kForegroundMask) != 0u; }
    };
}
