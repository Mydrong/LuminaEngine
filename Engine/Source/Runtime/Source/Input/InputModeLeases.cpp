#include "RuntimePCH.h"
#include "InputModeLeases.h"

#include "Input/InputQuery.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // Higher keeps more of the game away from the player, so the most guarded request among the leases wins.
        constexpr int32 InputModeRank(EInputMode Mode)
        {
            switch (Mode)
            {
                case EInputMode::UI:        return 2;
                case EInputMode::GameAndUI: return 1;
                default:                    return 0;
            }
        }

        // Higher shows more of the cursor, so one panel needing a pointer is enough to keep it visible.
        constexpr int32 MouseModeRank(EMouseMode Mode)
        {
            switch (Mode)
            {
                case EMouseMode::Normal: return 2;
                case EMouseMode::Hidden: return 1;
                default:                 return 0;
            }
        }
    }

    uint64 FInputModeLeases::Acquire(EInputMode InputMode, EMouseMode MouseMode)
    {
        if (Leases.empty())
        {
            BaseInputMode = Input::GetInputMode(World);
            BaseMouseMode = Input::GetMouseMode(World);
        }
        const uint64 Id = NextId++;
        Leases.push_back({ Id, InputMode, MouseMode });
        Apply();
        return Id;
    }

    bool FInputModeLeases::Release(uint64 Id)
    {
        for (size_t Index = 0; Index < Leases.size(); ++Index)
        {
            if (Leases[Index].Id == Id)
            {
                Leases.erase(Leases.begin() + Index);
                Apply();
                return true;
            }
        }
        return false;
    }

    void FInputModeLeases::Clear()
    {
        if (!Leases.empty())
        {
            Leases.clear();
            Apply();
        }
    }

    void FInputModeLeases::Apply()
    {
        if (Leases.empty())
        {
            Input::SetInputMode(World, BaseInputMode);
            Input::SetMouseMode(World, BaseMouseMode);
            return;
        }

        EInputMode InputMode = Leases[0].InputMode;
        EMouseMode MouseMode = Leases[0].MouseMode;
        for (const FLease& Lease : Leases)
        {
            if (InputModeRank(Lease.InputMode) > InputModeRank(InputMode))
            {
                InputMode = Lease.InputMode;
            }
            if (MouseModeRank(Lease.MouseMode) > MouseModeRank(MouseMode))
            {
                MouseMode = Lease.MouseMode;
            }
        }
        Input::SetInputMode(World, InputMode);
        Input::SetMouseMode(World, MouseMode);
    }

    FScopedInputMode::FScopedInputMode(CWorld* InWorld, EInputMode InputMode, EMouseMode MouseMode)
        : World(InWorld)
    {
        if (InWorld != nullptr)
        {
            Id = InWorld->GetInputModeLeases().Acquire(InputMode, MouseMode);
        }
    }

    FScopedInputMode::FScopedInputMode(FScopedInputMode&& Other) noexcept
        : World(std::move(Other.World))
        , Id(std::exchange(Other.Id, 0))
    {
    }

    FScopedInputMode& FScopedInputMode::operator=(FScopedInputMode&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset();
            World = std::move(Other.World);
            Id = std::exchange(Other.Id, 0);
        }
        return *this;
    }

    void FScopedInputMode::Reset()
    {
        if (Id != 0)
        {
            if (CWorld* Held = World.Get())
            {
                Held->GetInputModeLeases().Release(Id);
            }
            Id = 0;
        }
        World = nullptr;
    }
}
