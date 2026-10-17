#include "CategorizedPicker.h"

#include "imgui.h"
#include "Containers/Algorithm.h"
#include "Containers/StringFormat.h"
#include "Core/Math/Math.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"
#include "UI/Tools/NodeGraph/GraphActionMenu.h"

#include <cctype>

namespace Lumina
{
    namespace
    {
        constexpr float kPickerWidth      = 380.0f;
        constexpr float kPickerListHeight = 380.0f;

        // Up to this many items, every category starts open, since collapsing a short list only hides it.
        constexpr int32 kOpenAllBelowCount = 24;

        bool PickerContainsIgnoreCase(FStringView Haystack, const char* Needle)
        {
            const size_t NeedleLength = strlen(Needle);
            if (NeedleLength == 0 || NeedleLength > Haystack.size())
            {
                return NeedleLength == 0;
            }
            for (size_t Start = 0; Start + NeedleLength <= Haystack.size(); ++Start)
            {
                size_t Index = 0;
                while (Index < NeedleLength
                    && std::tolower((unsigned char)Haystack[Start + Index]) == std::tolower((unsigned char)Needle[Index]))
                {
                    ++Index;
                }
                if (Index == NeedleLength)
                {
                    return true;
                }
            }
            return false;
        }

        // The matched characters of a fuzzy search drawn in the accent color, coalesced into runs.
        void DrawPickerMatchedName(const FString& Name, const TVector<uint16>& Matches)
        {
            const ImVec4 MatchColor(1.0f, 0.82f, 0.35f, 1.0f);
            const char* Text = Name.c_str();
            const size_t Length = Name.size();

            size_t Cursor = 0;
            size_t MatchCursor = 0;
            bool bFirstRun = true;
            while (Cursor < Length)
            {
                const bool bMatched = MatchCursor < Matches.size() && Matches[MatchCursor] == Cursor;
                size_t RunEnd = Cursor + 1;
                if (bMatched)
                {
                    ++MatchCursor;
                    while (RunEnd < Length && MatchCursor < Matches.size() && Matches[MatchCursor] == RunEnd)
                    {
                        ++RunEnd;
                        ++MatchCursor;
                    }
                }
                else
                {
                    while (RunEnd < Length && (MatchCursor >= Matches.size() || Matches[MatchCursor] != RunEnd))
                    {
                        ++RunEnd;
                    }
                }

                if (!bFirstRun)
                {
                    ImGui::SameLine(0.0f, 0.0f);
                }
                bFirstRun = false;

                if (bMatched)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, MatchColor);
                }
                ImGui::TextUnformatted(Text + Cursor, Text + RunEnd);
                if (bMatched)
                {
                    ImGui::PopStyleColor();
                }
                Cursor = RunEnd;
            }
        }
    }

    const FPickerItem* FCategorizedPicker::DrawButton(const char* StrId, const char* Label, const FGatherItems& Gather, float Width)
    {
        const FPickerItem* Picked = nullptr;

        ImGui::PushID(StrId);
        if (ImGui::Button(Label, ImVec2(Width, 0.0f)))
        {
            ImGui::OpenPopup("##CategorizedPicker");
        }

        // Drops down from the button like a combo, and opens upward when the space below cannot hold it.
        const ImVec2 ButtonMin = ImGui::GetItemRectMin();
        const ImVec2 ButtonMax = ImGui::GetItemRectMax();
        const ImGuiViewport* Viewport = ImGui::GetWindowViewport();
        const float PopupHeight = kPickerListHeight + ImGui::GetFrameHeightWithSpacing() * 3.0f;
        const bool bOpenUpward = ButtonMax.y + PopupHeight > Viewport->WorkPos.y + Viewport->WorkSize.y;
        ImGui::SetNextWindowPos(bOpenUpward ? ButtonMin : ImVec2(ButtonMin.x, ButtonMax.y), ImGuiCond_Appearing, ImVec2(0.0f, bOpenUpward ? 1.0f : 0.0f));

        if (ImGui::BeginPopup("##CategorizedPicker"))
        {
            Picked = DrawContents(Gather);
            if (Picked != nullptr)
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

        return Picked;
    }

    const FPickerItem* FCategorizedPicker::DrawContents(const FGatherItems& Gather)
    {
        const bool bAppearing = ImGui::IsWindowAppearing();
        if (bAppearing)
        {
            Query[0] = '\0';
            Rebuild(Gather);
            Rescore();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
        ImGui::SetNextItemWidth(kPickerWidth);
        if (bAppearing)
        {
            ImGui::SetKeyboardFocusHere();
        }
        const FString Hint = Lumina::Format("{} {}", LE_ICON_MAGNIFY, SearchHint);
        const bool bEnter = ImGui::InputTextWithHint("##PickerSearch", Hint.c_str(), Query, sizeof(Query), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopStyleVar();

        if (ScoredQuery != Query)
        {
            Rescore();
        }

        if (!DrawnOrder.empty())
        {
            int32 Position = 0;
            while (Position < (int32)DrawnOrder.size() && DrawnOrder[Position] != SelectedItem)
            {
                ++Position;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
            {
                SelectedItem = DrawnOrder[Position >= (int32)DrawnOrder.size() ? 0 : (Position + 1) % (int32)DrawnOrder.size()];
                bScrollToSelected = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
            {
                SelectedItem = DrawnOrder[Position >= (int32)DrawnOrder.size() ? 0 : (Position + (int32)DrawnOrder.size() - 1) % (int32)DrawnOrder.size()];
                bScrollToSelected = true;
            }
        }

        int32 Picked = -1;
        DrawingOrder.clear();

        ImGui::Spacing();
        if (ImGui::BeginChild("##PickerList", ImVec2(kPickerWidth, kPickerListHeight)))
        {
            // Searching expands categories under its own ids, so the user's browsing layout survives clearing the query.
            ImGui::PushID(IsSearching() ? 1 : 0);
            if (!Categories.empty())
            {
                for (int32 Child : Categories[0].Children)
                {
                    DrawCategory(Child, Picked);
                }
                for (int32 Item : Categories[0].Items)
                {
                    DrawItem(Item, Picked);
                }
            }
            ImGui::PopID();

            if (DrawingOrder.empty())
            {
                ImGui::Spacing();
                ImGui::TextDisabled(Items.empty() ? "Nothing to pick from." : "No matches.");
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        const int32 Total = (int32)Items.size();
        const int32 Shown = IsSearching() ? CountItems(0, true) : Total;
        const FString Count = IsSearching()
            ? Lumina::Format("{} of {} {}", Shown, Total, ItemNoun)
            : Lumina::Format("{} {}", Total, ItemNoun);
        ImGui::TextDisabled("%s", Count.c_str());
        if (SelectedItem >= 0)
        {
            constexpr const char* EnterHint = "Enter to pick";
            ImGui::SameLine(kPickerWidth - ImGui::CalcTextSize(EnterHint).x);
            ImGui::TextDisabled("%s", EnterHint);
        }

        DrawnOrder = DrawingOrder;
        bScrollToSelected = false;

        if (bEnter && SelectedItem >= 0 && Algo::Contains(DrawnOrder, SelectedItem))
        {
            Picked = SelectedItem;
        }

        // Enter reaches a disabled item the same as the arrow keys do, so it is refused here rather than per input.
        return Picked >= 0 && Picked < (int32)Items.size() && !Items[Picked].bDisabled ? &Items[Picked] : nullptr;
    }

    void FCategorizedPicker::Rebuild(const FGatherItems& Gather)
    {
        Items.clear();
        if (Gather)
        {
            Gather(Items);
        }

        Algo::StableSort(Items, [](const FPickerItem& A, const FPickerItem& B) { return A.Name < B.Name; });

        Categories.clear();
        Categories.emplace_back();
        for (int32 Index = 0; Index < (int32)Items.size(); ++Index)
        {
            int32 Category = 0;
            const FStringView Path(Items[Index].Category);
            size_t Start = 0;
            while (Start < Path.size())
            {
                size_t End = Path.find('/', Start);
                if (End == FStringView::npos)
                {
                    End = Path.size();
                }
                if (End > Start)
                {
                    Category = FindOrAddChild(Category, Path.substr(Start, End - Start));
                }
                Start = End + 1;
            }
            Categories[Category].Items.push_back(Index);
        }

        CompressChains(0);
        for (FCategory& Category : Categories)
        {
            Algo::StableSort(Category.Children, [this](int32 A, int32 B) { return Categories[A].Label < Categories[B].Label; });
        }
        Algo::StableSort(Categories[0].Children, [this](int32 A, int32 B)
        {
            return Categories[A].Label != TrailingCategory && Categories[B].Label == TrailingCategory;
        });
        CountItems(0, false);

        bOpenCategoriesByDefault = Items.size() <= (size_t)kOpenAllBelowCount || Categories[0].Children.size() == 1;
        SelectedItem = -1;
        DrawnOrder.clear();
    }

    int32 FCategorizedPicker::FindOrAddChild(int32 Parent, FStringView Label)
    {
        for (int32 Child : Categories[Parent].Children)
        {
            if (FStringView(Categories[Child].Label) == Label)
            {
                return Child;
            }
        }

        const int32 Added = (int32)Categories.size();
        FCategory& Category = Categories.emplace_back();
        Category.Label = FString(Label.data(), Label.size());
        Categories[Parent].Children.push_back(Added);
        return Added;
    }

    void FCategorizedPicker::CompressChains(int32 CategoryIndex)
    {
        // A folder holding only another folder is one more click with nothing to choose, so the two share a row.
        if (CategoryIndex != 0)
        {
            while (Categories[CategoryIndex].Items.empty() && Categories[CategoryIndex].Children.size() == 1)
            {
                const int32 Only = Categories[CategoryIndex].Children[0];
                Categories[CategoryIndex].Label += " / " + Categories[Only].Label;
                Categories[CategoryIndex].Items = Categories[Only].Items;
                Categories[CategoryIndex].Children = Categories[Only].Children;
                Categories[Only].Children.clear();
                Categories[Only].Items.clear();
            }
        }

        const TVector<int32> Children = Categories[CategoryIndex].Children;
        for (int32 Child : Children)
        {
            CompressChains(Child);
        }
    }

    int32 FCategorizedPicker::CountItems(int32 CategoryIndex, bool bMatchesOnly)
    {
        int32 Count = 0;
        for (int32 Item : Categories[CategoryIndex].Items)
        {
            Count += !bMatchesOnly || ItemScores[Item] >= 0 ? 1 : 0;
        }
        const TVector<int32> Children = Categories[CategoryIndex].Children;
        for (int32 Child : Children)
        {
            Count += CountItems(Child, bMatchesOnly);
        }

        if (bMatchesOnly)
        {
            Categories[CategoryIndex].MatchCount = Count;
        }
        else
        {
            Categories[CategoryIndex].TotalCount = Count;
        }
        return Count;
    }

    void FCategorizedPicker::Rescore()
    {
        ScoredQuery = Query;
        ItemScores.assign(Items.size(), 0);
        ItemMatches.assign(Items.size(), TVector<uint16>());

        if (!IsSearching())
        {
            return;
        }

        int32 BestScore = -1;
        SelectedItem = -1;
        for (int32 Index = 0; Index < (int32)Items.size(); ++Index)
        {
            int32 Score = 0;
            TVector<uint16> Matches;
            if (FuzzyMatch(Query, Items[Index].Name.c_str(), Score, Matches))
            {
                // A hit on the name always outranks one that only matched the folder.
                ItemScores[Index]  = Score + 1000;
                ItemMatches[Index] = Move(Matches);
            }
            else if (PickerContainsIgnoreCase(FStringView(Items[Index].Category), Query))
            {
                ItemScores[Index] = 0;
            }
            else
            {
                ItemScores[Index] = -1;
            }

            if (ItemScores[Index] > BestScore)
            {
                BestScore    = ItemScores[Index];
                SelectedItem = Index;
            }
        }

        CountItems(0, true);
        bScrollToSelected = true;
    }

    void FCategorizedPicker::DrawCategory(int32 CategoryIndex, int32& OutPicked)
    {
        const FCategory& Category = Categories[CategoryIndex];
        if (IsSearching() && Category.MatchCount == 0)
        {
            return;
        }

        if (IsSearching())
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }
        else
        {
            ImGui::SetNextItemOpen(bOpenCategoriesByDefault, ImGuiCond_Once);
        }

        ImGui::PushID(CategoryIndex);
        const float RowRight = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
        const bool bOpen = ImGui::TreeNodeEx("##Category", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding);

        // Drawn over the node so the folder icon follows the open state and the count sits at the right edge.
        ImGui::SameLine();
        ImGui::TextUnformatted(bOpen ? LE_ICON_FOLDER_OPEN : LE_ICON_FOLDER);
        ImGui::SameLine();
        ImGui::TextUnformatted(Category.Label.c_str());

        const FString CountText = Lumina::Format("{}", IsSearching() ? Category.MatchCount : Category.TotalCount);
        ImGui::SameLine(RowRight - ImGui::CalcTextSize(CountText.c_str()).x - ImGui::GetStyle().FramePadding.x);
        ImGui::TextDisabled("%s", CountText.c_str());

        if (bOpen)
        {
            for (int32 Child : Category.Children)
            {
                DrawCategory(Child, OutPicked);
            }
            for (int32 Item : Category.Items)
            {
                DrawItem(Item, OutPicked);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void FCategorizedPicker::DrawItem(int32 ItemIndex, int32& OutPicked)
    {
        if (IsSearching() && ItemScores[ItemIndex] < 0)
        {
            return;
        }
        DrawingOrder.push_back(ItemIndex);

        const FPickerItem& Item = Items[ItemIndex];
        const bool bSelected = ItemIndex == SelectedItem;

        ImGui::PushID(ItemIndex);
        ImGui::BeginDisabled(Item.bDisabled);
        const ImVec2 RowStart = ImGui::GetCursorScreenPos();
        if (ImGui::Selectable("##Item", bSelected, ImGuiSelectableFlags_SpanAvailWidth, ImVec2(0.0f, ImGui::GetTextLineHeight() + 4.0f)))
        {
            OutPicked = ItemIndex;
        }
        const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        if (bSelected && bScrollToSelected)
        {
            ImGui::SetScrollHereY(0.5f);
        }

        // Name and icon sit over the row, indented like a tree leaf so they line up under their folder's label.
        ImGui::SetCursorScreenPos(ImVec2(RowStart.x + ImGui::GetTreeNodeToLabelSpacing() * 0.5f, RowStart.y + 2.0f));
        if (Item.Icon != nullptr)
        {
            ImGui::TextUnformatted(Item.Icon);
            ImGui::SameLine();
        }
        DrawPickerMatchedName(Item.Name, ItemMatches[ItemIndex]);
        ImGui::EndDisabled();

        if (bHovered && !Item.Tooltip.empty())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(Item.Name.c_str());
            ImGui::Separator();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
            ImGui::TextDisabled("%s", Item.Tooltip.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
}
