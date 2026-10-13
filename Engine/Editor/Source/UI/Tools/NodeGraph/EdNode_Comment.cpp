#include "EdNode_Comment.h"

#include "EdNodeGraph.h"
#include "Core/Object/Class.h"
#include "Core/Object/Package/Package.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"

namespace Lumina
{
    namespace
    {
        struct FCommentSwatch
        {
            const char* Name;
            FVector4    Color;
        };

        constexpr FCommentSwatch kCommentSwatches[] =
        {
            { "Slate",  FVector4(0.29f, 0.44f, 0.68f, 1.0f) },
            { "Teal",   FVector4(0.16f, 0.56f, 0.55f, 1.0f) },
            { "Green",  FVector4(0.32f, 0.58f, 0.30f, 1.0f) },
            { "Amber",  FVector4(0.86f, 0.63f, 0.22f, 1.0f) },
            { "Orange", FVector4(0.85f, 0.43f, 0.20f, 1.0f) },
            { "Red",    FVector4(0.76f, 0.27f, 0.30f, 1.0f) },
            { "Violet", FVector4(0.54f, 0.37f, 0.76f, 1.0f) },
            { "Gray",   FVector4(0.44f, 0.46f, 0.51f, 1.0f) },
        };
    }

    CClass* CEdNode_Comment::GetSupportedGraphClass() const
    {
        return CEdNodeGraph::StaticClass();
    }

    bool CEdNode_Comment::GetRenameText(FString& OutText) const
    {
        OutText = Title;
        return true;
    }

    void CEdNode_Comment::SetRenameText(const FString& InText)
    {
        Title = InText;
    }

    ImU32 CEdNode_Comment::GetTint(float Opacity) const
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(Color.x, Color.y, Color.z, Math::Clamp(Opacity, 0.0f, 1.0f)));
    }

    ImU32 CEdNode_Comment::GetTitleTextColor() const
    {
        const float Luminance = 0.2126f * Color.x + 0.7152f * Color.y + 0.0722f * Color.z;
        return Luminance > 0.58f ? IM_COL32(24, 26, 30, 255) : IM_COL32(246, 248, 251, 255);
    }

    void CEdNode_Comment::DrawContextMenu()
    {
        ImGui::TextDisabled(LE_ICON_PALETTE " Color");

        constexpr float SwatchSize = 18.0f;
        for (int32 Index = 0; Index < (int32)std::size(kCommentSwatches); ++Index)
        {
            const FCommentSwatch& Swatch = kCommentSwatches[Index];
            if (Index > 0)
            {
                ImGui::SameLine(0.0f, 4.0f);
            }

            ImGui::PushID(Index);
            const ImVec4 SwatchColor(Swatch.Color.x, Swatch.Color.y, Swatch.Color.z, 1.0f);
            if (ImGui::ColorButton(Swatch.Name, SwatchColor, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(SwatchSize, SwatchSize)))
            {
                Color = Swatch.Color;
                NotifyValueEdited();
                if (CEdNodeGraph* Graph = GetOwningGraph(); Graph != nullptr && Graph->GetPackage() != nullptr)
                {
                    Graph->GetPackage()->MarkDirty();
                }
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", Swatch.Name);
            }
            ImGui::PopID();
        }

        ImGui::Separator();
    }
}
