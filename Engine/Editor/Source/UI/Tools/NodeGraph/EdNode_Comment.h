#pragma once
#include "EdGraphNode.h"
#include "Core/Object/ObjectMacros.h"
#include "EdNode_Comment.generated.h"

namespace Lumina
{
    // A titled, tinted box drawn behind the graph; dragging its title bar carries every node inside it.
    REFLECT()
    class EDITOR_API CEdNode_Comment : public CEdGraphNode
    {
        GENERATED_BODY()
    public:

        // Every graph accepts one, since a comment compiles to nothing.
        CClass* GetSupportedGraphClass() const override;

        bool IsCommentNode() const override { return true; }
        bool WantsTitlebar() const override { return false; }

        FStringView GetNodeDisplayName() const override { return "Comment"; }
        FFixedString GetNodeCategory() const override { return "Utility"; }
        FStringView GetNodeTooltip() const override { return "A titled box that groups and explains part of the graph. Select nodes and press C to wrap them in one."; }
        FString GetNodeTitleText() const override { return Title; }

        bool GetRenameText(FString& OutText) const override;
        void SetRenameText(const FString& InText) override;

        void DrawContextMenu() override;

        // The tint with its alpha replaced by Opacity.
        ImU32 GetTint(float Opacity) const;

        // Dark text on a light tint and light text on a dark one.
        ImU32 GetTitleTextColor() const;

        PROPERTY(Editable, Category = "Comment")
        FString Title = "Comment";

        PROPERTY(Editable, Category = "Comment", Multiline)
        FString Description;

        PROPERTY(Editable, Color, Category = "Comment")
        FVector4 Color = FVector4(0.29f, 0.44f, 0.68f, 1.0f);

        // The body under the title bar in canvas units, mirrored from the canvas so undo and saves carry a resize.
        PROPERTY()
        float Width = 480.0f;

        PROPERTY()
        float Height = 280.0f;
    };
}
