#include "EntityScriptPicker.h"

#include "Containers/HashTable.h"
#include "Containers/StringFormat.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Object/ScriptClass.h"
#include "Scripting/DotNet/DotNetHost.h"
#include "Scripting/EntityScript.h"
#include "Tools/UI/ImGui/ImGuiDesignIcons.h"

#include <cctype>

namespace Lumina::EntityScriptPicker
{
    namespace
    {
        FString NormalizeSourceKey(FStringView Text)
        {
            FString Lower(Text.data(), Text.size());
            for (char& Character : Lower)
            {
                Character = Character == '\\' ? '/' : (char)std::tolower((unsigned char)Character);
            }
            return Lower;
        }
    }

    void GatherItems(TVector<FPickerItem>& Out)
    {
        Out.clear();

        // One pass over the script roots, since a lookup per class would walk them once for every row.
        TVector<DotNet::FScriptSourceFolder> Sources;
        DotNet::GatherScriptSourceFolders(Sources);
        THashMap<FString, const DotNet::FScriptSourceFolder*> SourceByStem;
        THashMap<FString, const DotNet::FScriptSourceFolder*> SourceByPath;
        for (const DotNet::FScriptSourceFolder& Source : Sources)
        {
            SourceByStem.emplace(NormalizeSourceKey(Source.Stem), &Source);
            SourceByPath.emplace(NormalizeSourceKey(Source.Path), &Source);
        }

        CClass* Base = CEntityScript::StaticClass();
        for (TObjectIterator<CClass> It; It; ++It)
        {
            CClass* Class = *It;
            if (Class == nullptr || Class == Base || !Class->IsChildOf(Base)
                || Class->HasAnyFlag(OF_DefaultObject) || Class->HasAnyFlag(OF_MarkedDestroy)
                || Class->IsAbstract() || Class->IsSuperseded())
            {
                continue;
            }

            const FString FullName = Class->GetName().ToString();
            const FStringView View(FullName);
            const size_t Dot = View.find_last_of('.');
            const FStringView Short = Dot == FStringView::npos ? View : View.substr(Dot + 1);
            const bool bManaged = ToScriptClass(Class) != nullptr;

            FPickerItem& Item = Out.emplace_back();
            Item.Name = Class->HasMeta("DisplayName") ? Class->GetMeta("DisplayName") : FString(Short.data(), Short.size());
            Item.Key  = FullName;
            Item.Icon = bManaged ? LE_ICON_LANGUAGE_CSHARP : LE_ICON_LANGUAGE_CPP;

            const DotNet::FScriptSourceFolder* Source = nullptr;
            if (bManaged && Class->HasMeta("SourceFile"))
            {
                auto ByPath = SourceByPath.find(NormalizeSourceKey(Class->GetMeta("SourceFile")));
                if (ByPath != SourceByPath.end())
                {
                    Source = ByPath->second;
                }
            }
            if (bManaged && Source == nullptr)
            {
                auto ByStem = SourceByStem.find(NormalizeSourceKey(Short));
                if (ByStem != SourceByStem.end())
                {
                    Source = ByStem->second;
                }
            }
            if (Source != nullptr)
            {
                Item.Category = Source->Folder;
                Item.Tooltip  = Lumina::Format("{}\n{}", FullName, Source->Path);
            }
            else
            {
                // A C++ script has no file under the script roots, so it files under its own heading.
                Item.Category = bManaged ? FString() : FString(NativeCategory);
                Item.Tooltip  = FullName;
            }

            // A declared REFLECT(Category) or ToolTip wins over where the source file happens to sit.
            if (Class->HasMeta("Category"))
            {
                Item.Category = Class->GetMeta("Category");
            }
            if (Class->HasMeta("ToolTip"))
            {
                Item.Tooltip = Lumina::Format("{}\n{}", Class->GetMeta("ToolTip"), Item.Tooltip);
            }
        }
    }
}
