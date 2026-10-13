#include "ReflectionSpecifiers.h"

#include <algorithm>
#include <charconv>
#include <array>
#include <ranges>
#include "Reflector/Diagnostics/LRTDiagnostics.h"

namespace Lumina::Reflection
{
    namespace
    {
#define LUMINA_SPECIFIER_ROW(Name, Form, Consumer, Doc) \
    { #Name, ESpecifierForm::Form, ESpecifierConsumer::Consumer, Doc },

        // Sorted during constant evaluation so every lookup is a binary search over a read-only table.
        template<typename TTable>
        constexpr TTable SortedByName(TTable Table)
        {
            std::ranges::sort(Table, {}, &FSpecifierInfo::Name);
            return Table;
        }

        constexpr auto GReflectSpecifiers = SortedByName(
            std::to_array<FSpecifierInfo>({ LUMINA_REFLECT_SPECIFIERS(LUMINA_SPECIFIER_ROW) }));
        constexpr auto GPropertySpecifiers = SortedByName(
            std::to_array<FSpecifierInfo>({ LUMINA_PROPERTY_SPECIFIERS(LUMINA_SPECIFIER_ROW) }));
        constexpr auto GFunctionSpecifiers = SortedByName(
            std::to_array<FSpecifierInfo>({ LUMINA_FUNCTION_SPECIFIERS(LUMINA_SPECIFIER_ROW) }));
        constexpr auto GScriptExportSpecifiers = SortedByName(
            std::to_array<FSpecifierInfo>({ LUMINA_SCRIPT_EXPORT_SPECIFIERS(LUMINA_SPECIFIER_ROW) }));

#undef LUMINA_SPECIFIER_ROW

        // A duplicate row would make one of the two spellings unreachable through the binary search.
        static_assert(std::ranges::adjacent_find(GReflectSpecifiers, {}, &FSpecifierInfo::Name) == GReflectSpecifiers.end());
        static_assert(std::ranges::adjacent_find(GPropertySpecifiers, {}, &FSpecifierInfo::Name) == GPropertySpecifiers.end());
        static_assert(std::ranges::adjacent_find(GFunctionSpecifiers, {}, &FSpecifierInfo::Name) == GFunctionSpecifiers.end());
        static_assert(std::ranges::adjacent_find(GScriptExportSpecifiers, {}, &FSpecifierInfo::Name) == GScriptExportSpecifiers.end());

        struct FPropertyFlagSpecifier
        {
            std::string_view Name;
            EPropertyFlags   Flag;
        };

        constexpr FPropertyFlagSpecifier GPropertyFlagSpecifiers[] =
        {
            { "Editable",           EPropertyFlags::Editable },
            { "ReadOnly",           EPropertyFlags::ReadOnly },
            { "NoSerialize",        EPropertyFlags::NoSerialize },
            { "EditorOnly",         EPropertyFlags::EditorOnly },
            { "Replicated",         EPropertyFlags::Replicated },
            { "Sync",               EPropertyFlags::Replicated },
            { "DuplicateTransient", EPropertyFlags::DuplicateTransient },
            { "SaveGame",           EPropertyFlags::SaveGame },
            { "ScriptReadOnly",     EPropertyFlags::ScriptReadOnly },
            { "ScriptWritable",     EPropertyFlags::ScriptWritable },
            { "ScriptHidden",       EPropertyFlags::ScriptHidden },
            { "NotScriptable",      EPropertyFlags::ScriptHidden },
        };

        constexpr size_t kMaxSpecifierLength = 64;

        // Only reached for a specifier that already failed lookup, so one stack row beats a heap allocation.
        uint32_t EditDistance(std::string_view A, std::string_view B)
        {
            if (B.size() >= kMaxSpecifierLength)
            {
                return kMaxSpecifierLength;
            }

            uint32_t Row[kMaxSpecifierLength + 1];
            for (uint32_t J = 0; J <= B.size(); ++J)
            {
                Row[J] = J;
            }

            for (uint32_t I = 1; I <= A.size(); ++I)
            {
                uint32_t Diagonal = Row[0];
                Row[0] = I;

                for (uint32_t J = 1; J <= B.size(); ++J)
                {
                    const uint32_t Previous = Row[J];
                    const uint32_t Cost = (A[I - 1] == B[J - 1]) ? 0u : 1u;
                    Row[J] = std::min(std::min(Row[J] + 1, Row[J - 1] + 1), Diagonal + Cost);
                    Diagonal = Previous;
                }
            }

            return Row[B.size()];
        }
    }

    const char* SpecifierTargetToString(ESpecifierTarget Target)
    {
        switch (Target)
        {
            case ESpecifierTarget::Reflect:      return "REFLECT";
            case ESpecifierTarget::Property:     return "PROPERTY";
            case ESpecifierTarget::Function:     return "FUNCTION";
            case ESpecifierTarget::ScriptExport: return "SCRIPT_EXPORT";
            default:                             return "NONE";
        }
    }

    std::span<const FSpecifierInfo> GetSpecifiers(ESpecifierTarget Target)
    {
        switch (Target)
        {
            case ESpecifierTarget::Reflect:      return GReflectSpecifiers;
            case ESpecifierTarget::Property:     return GPropertySpecifiers;
            case ESpecifierTarget::Function:     return GFunctionSpecifiers;
            case ESpecifierTarget::ScriptExport: return GScriptExportSpecifiers;
            default:                             return {};
        }
    }

    const FSpecifierInfo* FindSpecifier(ESpecifierTarget Target, std::string_view Key)
    {
        const std::span<const FSpecifierInfo> Table = GetSpecifiers(Target);

        const auto Found = std::ranges::lower_bound(Table, Key, {}, &FSpecifierInfo::Name);
        return Found != Table.end() && Found->Name == Key ? &*Found : nullptr;
    }

    const FSpecifierInfo* SuggestSpecifier(ESpecifierTarget Target, std::string_view Key)
    {
        // Loose enough to catch a plausible misspelling, tight enough that unrelated names never match.
        const uint32_t MaxDistance = Key.length() <= 4 ? 1u : (Key.length() <= 8 ? 2u : 3u);

        const FSpecifierInfo* Best = nullptr;
        uint32_t BestDistance = MaxDistance + 1;

        for (const FSpecifierInfo& Candidate : GetSpecifiers(Target))
        {
            const uint32_t Distance = EditDistance(Key, Candidate.Name);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                Best = &Candidate;
            }
        }

        return BestDistance <= MaxDistance ? Best : nullptr;
    }

    bool ParseSpecifierNumber(std::string_view Text, double& OutValue)
    {
        if (!Text.empty() && Text.front() == '+')
        {
            Text.remove_prefix(1);
        }
        if (!Text.empty() && (Text.back() == 'f' || Text.back() == 'F'))
        {
            Text.remove_suffix(1);
        }

        const auto [End, Error] = std::from_chars(Text.data(), Text.data() + Text.size(), OutValue);
        return !Text.empty() && Error == std::errc() && End == Text.data() + Text.size();
    }

    EPropertyFlags PropertyFlagForSpecifier(std::string_view Key)
    {
        for (const FPropertyFlagSpecifier& Entry : GPropertyFlagSpecifiers)
        {
            if (Entry.Name == Key)
            {
                return Entry.Flag;
            }
        }
        return EPropertyFlags::None;
    }

    ESpecifierStorage GetSpecifierStorage(ESpecifierTarget Target, std::string_view Key)
    {
        const FSpecifierInfo* Info = FindSpecifier(Target, Key);
        if (Info == nullptr)
        {
            return ESpecifierStorage::Metadata;
        }

        if (Info->Consumer == ESpecifierConsumer::Reflector || Info->Consumer == ESpecifierConsumer::Script)
        {
            return ESpecifierStorage::None;
        }

        // A bare flag is all the bit says, while Sync = FromOwner still needs its value at runtime.
        const bool bBareFlag = Info->Form == ESpecifierForm::Flag;
        if (Target == ESpecifierTarget::Property && bBareFlag && PropertyFlagForSpecifier(Key) != EPropertyFlags::None)
        {
            return ESpecifierStorage::None;
        }

        if (Info->Consumer == ESpecifierConsumer::Editor)
        {
            return Target == ESpecifierTarget::Property && bBareFlag ? ESpecifierStorage::EditorFlag : ESpecifierStorage::EditorMetadata;
        }

        return ESpecifierStorage::Metadata;
    }

    void ValidateSpecifiers(const CXCursor& Cursor, ESpecifierTarget Target, const std::vector<FMetadataPair>& Metadata)
    {
        const char* MacroName = SpecifierTargetToString(Target);

        for (const FMetadataPair& Pair : Metadata)
        {
            if (Pair.Key.empty())
            {
                continue;
            }

            if (const FSpecifierInfo* Info = FindSpecifier(Target, Pair.Key))
            {
                double Number = 0.0;
                if (Info->Form == ESpecifierForm::Flag && !Pair.Value.empty())
                {
                    LRT_WARNING(Cursor, EDiagId::SpecifierForm,
                        "%s specifier '%s' takes no value, so '%s' is ignored. Write it as a bare '%s'.",
                        MacroName, Pair.Key.c_str(), Pair.Value.c_str(), Pair.Key.c_str());
                }
                else if ((Info->Form == ESpecifierForm::Value || Info->Form == ESpecifierForm::Number) && Pair.Value.empty())
                {
                    LRT_WARNING(Cursor, EDiagId::SpecifierForm,
                        "%s specifier '%s' needs a value, as in '%s = ...', and does nothing without one.",
                        MacroName, Pair.Key.c_str(), Pair.Key.c_str());
                }
                else if (Info->Form == ESpecifierForm::Number && !ParseSpecifierNumber(Pair.Value, Number))
                {
                    LRT_ERROR(Cursor, EDiagId::SpecifierForm,
                        "%s specifier '%s' must be a number, but '%s' is not one.",
                        MacroName, Pair.Key.c_str(), Pair.Value.c_str());
                }
                continue;
            }

            if (const FSpecifierInfo* Suggestion = SuggestSpecifier(Target, Pair.Key))
            {
                LRT_WARNING(Cursor, EDiagId::UnknownSpecifier,
                    "%s specifier '%s' is not recognized and will be ignored. Did you mean '%s'?",
                    MacroName, Pair.Key.c_str(), std::string(Suggestion->Name).c_str());
            }
            else
            {
                LRT_WARNING(Cursor, EDiagId::UnknownSpecifier,
                    "%s specifier '%s' is not recognized and will be ignored. Add it to ReflectionSpecifiers.h if it is intentional.",
                    MacroName, Pair.Key.c_str());
            }
        }
    }
}
