#include "RuntimePCH.h"
#include "ConsoleVariable.h"
#include <nlohmann/json.hpp>
#include "Core/Assertions/Assert.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"
#include "Containers/StringFormat.h"


namespace Lumina
{
    FConsoleRegistry& FConsoleRegistry::Get() noexcept
    {
        static FConsoleRegistry Instance;
        return Instance;
    }

    void FConsoleRegistry::Register(FConsoleVariable&& Var) noexcept
    {
        FStringView VarName = Var.Name.data();
        ConsoleVariables.emplace(VarName, Move(Var));
    }

    void FConsoleRegistry::RegisterCommand(FConsoleCommand&& Cmd) noexcept
    {
        FStringView CmdName = Cmd.Name.data();
        ConsoleCommands.emplace(CmdName, Move(Cmd));
    }

    FConsoleVariable* FConsoleRegistry::Find(FStringView Name)
    {
        auto It = ConsoleVariables.find(Name);
        return It != ConsoleVariables.end() ? &It->second : nullptr;
    }

    FConsoleCommand* FConsoleRegistry::FindCommand(FStringView Name)
    {
        auto It = ConsoleCommands.find(Name);
        return It != ConsoleCommands.end() ? &It->second : nullptr;
    }

    const FConsoleRegistry::FConsoleContainer& FConsoleRegistry::GetAll() const
    {
        return ConsoleVariables;
    }

    const FConsoleRegistry::FCommandContainer& FConsoleRegistry::GetAllCommands() const
    {
        return ConsoleCommands;
    }

    bool FConsoleRegistry::ExecuteCommand(FStringView Name)
    {
        FConsoleCommand* Cmd = FindCommand(Name);
        if (Cmd == nullptr || Cmd->Execute == nullptr)
        {
            return false;
        }

        Cmd->Execute();
        return true;
    }

    FConsoleLineResult FConsoleRegistry::ExecuteLine(FStringView Line, bool bEchoToLog)
    {
        while (!Line.empty() && (Line.front() == ' ' || Line.front() == '\t'))
        {
            Line.remove_prefix(1);
        }
        while (!Line.empty() && (Line.back() == ' ' || Line.back() == '\t'))
        {
            Line.remove_suffix(1);
        }

        FConsoleLineResult Result;
        if (Line.empty())
        {
            return Result;
        }
        if (bEchoToLog)
        {
            LOG_INFO("> {}", Line);
        }

        const size_t Space = Line.find_first_of(' ');
        const FStringView Name = Space == FStringView::npos ? Line : Line.substr(0, Space);
        const FStringView Value = Space == FStringView::npos ? FStringView() : Line.substr(Space + 1);
        Result.Name.assign(Name.data(), Name.size());

        if (FindCommand(Name) != nullptr)
        {
            if (bEchoToLog && !Value.empty())
            {
                LOG_WARN("'{}' is a command and takes no arguments; ignoring '{}'", Name, Value);
            }
            ExecuteCommand(Name);
            Result.Kind = EConsoleLineResult::RanCommand;
            return Result;
        }

        if (Find(Name) == nullptr)
        {
            if (bEchoToLog)
            {
                LOG_WARN("Unknown console command or variable: '{}'", Name);
            }
            Result.Kind = EConsoleLineResult::Unknown;
            return Result;
        }

        if (!Value.empty() && !SetValueFromString(Name, Value))
        {
            if (bEchoToLog)
            {
                LOG_WARN("Failed to set '{}' to '{}'", Name, Value);
            }
            Result.Kind = EConsoleLineResult::RejectedValue;
            Result.Value.assign(Value.data(), Value.size());
            return Result;
        }

        const TOptional<FString> Current = GetValueAsString(Name);
        Result.Value = Current.has_value() ? *Current : FString("<unprintable>");
        Result.Kind = Value.empty() ? EConsoleLineResult::ReadVariable : EConsoleLineResult::SetVariable;
        if (bEchoToLog)
        {
            LOG_INFO("{} = {}", Name, Result.Value);
        }
        return Result;
    }

    bool FConsoleRegistry::SetValueFromString(FStringView TargetName, FStringView StrValue)
    {
        FConsoleVariable* ConsoleVar = Find(TargetName);
        if (ConsoleVar == nullptr)
        {
            return false;
        }

        bool bSuccess = visit([&]<typename T0>(T0&&) -> bool
        {
            using T = std::decay_t<T0>;
            
            TOptional<T> ParsedValue = ConsoleHelpers::ParseValue<T>(StrValue);
            if (ParsedValue.has_value())
            {
                *(ConsoleVar->ValuePtr) = *ParsedValue;
                return true;
            }
            
            return false;
        }, *(ConsoleVar->ValuePtr));

        if (bSuccess && ConsoleVar->OnChange)
        {
            ConsoleVar->OnChange(*(ConsoleVar->ValuePtr));
            SaveToConfig();
        }

        return bSuccess;
    }
    
    TOptional<FString> FConsoleRegistry::GetValueAsString(FStringView VariableName)
    {
        FConsoleVariable* ConsoleVar = Find(VariableName);
        if (ConsoleVar == nullptr)
        {
            return NullOpt;
        }

        FString Result;

        bool bSuccess = visit([&]<typename T0>(T0&& Value) -> bool
        {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, int>)
            {
                Result = FString(Format("{}", Value));
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                Result = FString(Format("{}", Value));
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                Result = FString(Format("{}", Value));
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                Result = Value ? "true" : "false";
            }
            else if constexpr (std::is_same_v<T, FString>)
            {
                Result = Value;
            }
            else
            {
                // Unsupported type
                return false;
            }

            return true;
        }, *(ConsoleVar->ValuePtr));

        if (!bSuccess)
        {
            return NullOpt;
        }

        return Result;
    }

    void FConsoleRegistry::SaveToConfig()
    {
        // TODO serialize console variables to the config directory.
    }

    void FConsoleRegistry::LoadFromConfig()
    {
        // TODO restore console variables from the config directory.
    }
}
