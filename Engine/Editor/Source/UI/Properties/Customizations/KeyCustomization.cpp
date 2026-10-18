#include "Containers/StringFormat.h"
#include <iterator>
#include "CoreTypeCustomization.h"
#include "AssetPickerFilter.h"

#include "imgui.h"
#include "Input/InputViewport.h"
#include "Tools/UI/ImGui/EditorColors.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "Tools/UI/ImGui/ImGuiKeyCapture.h"
#include "Tools/UI/ImGui/ImGuiX.h"

namespace Lumina
{
    namespace
    {
        FString KeyBindingText(const SKey& Key)
        {
            if (!Key.IsValid())
            {
                return FString("Unbound");
            }

            const char* Icon = Key.IsGamepad() ? LE_ICON_GAMEPAD_VARIANT : Key.IsMouse() ? LE_ICON_MOUSE : LE_ICON_KEYBOARD;
            return FString(Icon) + "  " + Key.GetDisplayName();
        }
    }

    EPropertyChangeOp FKeyPropertyCustomization::DrawProperty(const TSharedPtr<FPropertyHandle>& Property, const FPropertyDrawArgs& Args)
    {
        bool bCommitted = false; // a new binding was set (or cleared) this frame

        const ImGuiStyle& St     = ImGui::GetStyle();
        const bool        bValid = DisplayValue.IsValid();

        // Kept at full width while listening, so the field does not jump as the clear button comes and goes.
        const float ClearW = AssetPickerFilter::kButtonSize.x + St.ItemSpacing.x;
        const float FieldW = ImGui::GetContentRegionAvail().x - ClearW;

        FFixedString Label;
        AppendFormat(Label, "{}##skey",
            bCapturing ? LE_ICON_KEYBOARD "  Press a key, mouse button or gamepad button..." : KeyBindingText(DisplayValue).c_str());

        // Styled as a value field rather than a button, since it shows the binding the way its neighbors show theirs.
        ImGui::PushStyleColor(ImGuiCol_Button,        St.Colors[ImGuiCol_FrameBg]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, St.Colors[ImGuiCol_FrameBgHovered]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  St.Colors[ImGuiCol_FrameBgActive]);
        ImGui::PushStyleColor(ImGuiCol_Text, bCapturing ? EditorColors::Accent() : bValid ? St.Colors[ImGuiCol_Text] : EditorColors::TextMuted());
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        if (ImGui::Button(Label.c_str(), ImVec2(FieldW > 1.0f ? FieldW : 1.0f, 0.0f)) && !bCapturing)
        {
            bCapturing      = true;
            bArmed          = false;
            PendingModifier = EKey::Num;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGuiX::TextTooltip("{}", bCapturing ? "Press a key, mouse button or gamepad button. Esc cancels."
                                             : "Click, then press a key, mouse button or gamepad button to rebind");

        // Disabled rather than hidden while listening, or the activating click would land on it.
        ImGui::SameLine(0.0f, St.ItemSpacing.x);
        ImGui::BeginDisabled(!bValid || bCapturing);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.5f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button(LE_ICON_CLOSE_CIRCLE "##clearskey", AssetPickerFilter::kButtonSize))
        {
            DisplayValue.Clear();
            bCapturing = false;
            bCommitted = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();
        ImGuiX::TextTooltip("{}", "Clear the binding.");

        if (bCapturing)
        {
            if (!bArmed)
            {
                // Skip the frame the activating click landed on; otherwise that click binds immediately.
                bArmed = true;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Escape, /*repeat*/ false))
            {
                bCapturing = false; // cancel, leave the binding unchanged
            }
            else
            {
                // Bare modifier presses keep listening, so a chord can be built before the key commits.
                const ImGuiIO& Io = ImGui::GetIO();
                const EKey K = ImGuiX::PollPressedKey();
                if (K != EKey::Num && ImGuiX::IsModifierEKey(K))
                {
                    PendingModifier = K;
                }
                else if (K != EKey::Num)
                {
                    DisplayValue.SetKey(K);
                    DisplayValue.bCtrl  = Io.KeyCtrl;
                    DisplayValue.bShift = Io.KeyShift;
                    DisplayValue.bAlt   = Io.KeyAlt;
                    bCapturing = false;
                    bCommitted = true;
                }
                else if (const EMouseKey M = ImGuiX::PollPressedMouseButton(); M != EMouseKey::Num)
                {
                    DisplayValue.SetMouseButton(M);
                    DisplayValue.bCtrl  = Io.KeyCtrl;
                    DisplayValue.bShift = Io.KeyShift;
                    DisplayValue.bAlt   = Io.KeyAlt;
                    bCapturing = false;
                    bCommitted = true;
                }
                else if (const EGamepadButton G = FInputViewportRegistry::Get().GetGamepads().GetFirstPressedButton(); G != EGamepadButton::Num)
                {
                    DisplayValue.SetGamepadButton(G);
                    bCapturing = false;
                    bCommitted = true;
                }
                else if (PendingModifier != EKey::Num && !ImGuiX::IsModifierHeld(PendingModifier))
                {
                    // Released with nothing pressed alongside it, so the modifier itself is the binding, such as Ctrl for crouch.
                    DisplayValue.SetKey(PendingModifier);
                    DisplayValue.bCtrl  = false;
                    DisplayValue.bShift = false;
                    DisplayValue.bAlt   = false;
                    bCapturing = false;
                    bCommitted = true;
                }
            }
        }

        // A discrete-edit transaction, opened on the change frame and closed the next.
        if (bCommitted)
        {
            bFinishPending = true;
            return EPropertyChangeOp::Started;
        }
        if (bFinishPending)
        {
            bFinishPending = false;
            return EPropertyChangeOp::Finished;
        }
        return EPropertyChangeOp::None;
    }

    void FKeyPropertyCustomization::UpdatePropertyValue(const TSharedPtr<FPropertyHandle>& Property)
    {
        CachedValue = DisplayValue;
        Property->SetValue(CachedValue);
    }

    void FKeyPropertyCustomization::HandleExternalUpdate(const TSharedPtr<FPropertyHandle>& Property)
    {
        SKey ActualValue;
        Property->GetValue(&ActualValue);

        // An external change must win even mid-listen, or the next write puts the stale value back.
        if (CachedValue != ActualValue)
        {
            CachedValue = DisplayValue = ActualValue;
            bCapturing  = false;
            bArmed      = false;
        }
    }
}
