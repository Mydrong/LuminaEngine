#ifdef _WIN32

#include <Windows.h>
#include <shellapi.h>

#include "Containers/String.h"
#include "Containers/Vector.h"

#pragma comment(lib, "Shell32.lib")

#include "CpuCheck.h"

extern int LuminaMain(int ArgC, char** ArgV);

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ char* pCmdLine, _In_ int nCmdShow)
{
    if (!LuminaHasRequiredCpu())
    {
        LuminaReportUnsupportedCpu();
        return 1;
    }

    int ArgC = 0;
    LPWSTR* WideArgs = CommandLineToArgvW(GetCommandLineW(), &ArgC);
    if (WideArgs == nullptr)
    {
        return 1;
    }

    Lumina::TVector<Lumina::FString> Utf8Args;
    Utf8Args.reserve(ArgC);
    for (int Index = 0; Index < ArgC; ++Index)
    {
        const int Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, WideArgs[Index], -1, nullptr, 0, nullptr, nullptr);
        if (Length == 0)
        {
            LocalFree(WideArgs);
            return 1;
        }

        Lumina::FString& Arg = Utf8Args.emplace_back(static_cast<size_t>(Length), '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, WideArgs[Index], -1, Arg.data(), Length, nullptr, nullptr) == 0)
        {
            LocalFree(WideArgs);
            return 1;
        }
        Arg.pop_back();
    }
    LocalFree(WideArgs);

    Lumina::TVector<char*> ArgV;
    ArgV.reserve(Utf8Args.size());
    for (Lumina::FString& Arg : Utf8Args)
    {
        ArgV.push_back(Arg.data());
    }
    return LuminaMain(ArgC, ArgV.data());
}

#endif
