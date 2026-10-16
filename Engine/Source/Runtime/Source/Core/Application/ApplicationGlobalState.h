#pragma once
namespace Lumina
{
    struct RUNTIME_API FApplicationGlobalState
    {
        FApplicationGlobalState(char const* MainThreadName = nullptr);
        ~FApplicationGlobalState();
        FApplicationGlobalState(const FApplicationGlobalState&) = delete;
        FApplicationGlobalState& operator=(const FApplicationGlobalState&) = delete;
        FApplicationGlobalState(FApplicationGlobalState&&) = delete;
        FApplicationGlobalState& operator=(FApplicationGlobalState&&) = delete;
    };
}
