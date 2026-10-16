#include "CoreTypeCustomization.h"
#include "imgui.h"
#include "Containers/StringFormat.h"
#include "Renderer/RenderLayers.h"

namespace Lumina
{
    namespace
    {
        FFixedString LayerLabel(uint32 Layer)
        {
            const FName Name = SRenderLayerMask::GetLayerName((int32)Layer);
            FFixedString Label;
            if (Name != NAME_None)
            {
                Label = Name.c_str();
            }
            else
            {
                AppendFormat(Label, "Layer {}", Layer);
            }
            return Label;
        }

        FFixedString MaskPreview(uint32 Bits)
        {
            if (Bits == RenderLayers::kAllMask)
            {
                return FFixedString("Everything");
            }
            if (Bits == 0u)
            {
                return FFixedString("Nothing");
            }

            // Leaves room for the ellipsis, since the fixed string cannot grow past its capacity.
            constexpr SIZE_T MaxPreview = 200;
            FFixedString Preview;
            for (uint32 Layer = 0; Layer < RenderLayers::kCount; ++Layer)
            {
                if ((Bits & (1u << Layer)) == 0u)
                {
                    continue;
                }
                const FFixedString Label = LayerLabel(Layer);
                if (Preview.size() + Label.size() + 2 > MaxPreview)
                {
                    Preview += ", ...";
                    break;
                }
                if (!Preview.empty())
                {
                    Preview += ", ";
                }
                Preview += Label.c_str();
            }
            return Preview;
        }
    }

    EPropertyChangeOp FRenderLayerMaskPropertyCustomization::DrawProperty(const TSharedPtr<FPropertyHandle>& Property, const FPropertyDrawArgs& Args)
    {
        bool bChanged = false;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##layers", MaskPreview(CachedBits).c_str(), ImGuiComboFlags_HeightLarge))
        {
            if (ImGui::Selectable("Everything", CachedBits == RenderLayers::kAllMask))
            {
                CachedBits = RenderLayers::kAllMask;
                bChanged   = true;
            }
            if (ImGui::Selectable("Nothing", CachedBits == 0u))
            {
                CachedBits = 0u;
                bChanged   = true;
            }
            ImGui::Separator();

            for (uint32 Layer = 0; Layer < RenderLayers::kCount; ++Layer)
            {
                const uint32 Bit = 1u << Layer;

                // An unnamed layer is listed only while this mask holds it, so it can still be cleared.
                if (SRenderLayerMask::GetLayerName((int32)Layer) == NAME_None && (CachedBits & Bit) == 0u)
                {
                    continue;
                }

                bool bSet = (CachedBits & Bit) != 0u;
                ImGui::PushID((int)Layer);
                if (ImGui::Checkbox(LayerLabel(Layer).c_str(), &bSet))
                {
                    CachedBits = bSet ? (CachedBits | Bit) : (CachedBits & ~Bit);
                    bChanged   = true;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        return EditSession.Advance(bChanged, false);
    }

    void FRenderLayerMaskPropertyCustomization::UpdatePropertyValue(const TSharedPtr<FPropertyHandle>& Property)
    {
        Property->SetValue(SRenderLayerMask(CachedBits));
    }

    void FRenderLayerMaskPropertyCustomization::HandleExternalUpdate(const TSharedPtr<FPropertyHandle>& Property)
    {
        SRenderLayerMask Value;
        Property->GetValue(&Value);
        CachedBits = Value.Bits;
    }
}
