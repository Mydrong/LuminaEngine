#pragma once

#include "imgui.h"
#include "Core/LuminaMacros.h"

namespace Lumina::ImGuiX
{
    RUNTIME_API bool DragScalar(const char* Label, ImGuiDataType DataType, void* Data, float Speed = 1.0f,
        const void* Min = nullptr, const void* Max = nullptr, const char* Format = nullptr, ImGuiSliderFlags Flags = 0);
    RUNTIME_API bool DragScalarN(const char* Label, ImGuiDataType DataType, void* Data, int Components, float Speed = 1.0f,
        const void* Min = nullptr, const void* Max = nullptr, const char* Format = nullptr, ImGuiSliderFlags Flags = 0);
    RUNTIME_API bool InputScalar(const char* Label, ImGuiDataType DataType, void* Data, const void* Step = nullptr,
        const void* StepFast = nullptr, const char* Format = nullptr, ImGuiInputTextFlags Flags = 0);
    RUNTIME_API bool InputScalarN(const char* Label, ImGuiDataType DataType, void* Data, int Components, const void* Step = nullptr,
        const void* StepFast = nullptr, const char* Format = nullptr, ImGuiInputTextFlags Flags = 0);
    RUNTIME_API bool DragFloat2(const char* Label, float Data[2], float Speed = 1.0f, float Min = 0.0f, float Max = 0.0f,
        const char* Format = "%.3f", ImGuiSliderFlags Flags = 0);
    RUNTIME_API bool DragFloat(const char* Label, float* Data, float Speed = 1.0f, float Min = 0.0f, float Max = 0.0f,
        const char* Format = "%.3f", ImGuiSliderFlags Flags = 0);
    RUNTIME_API bool DragInt(const char* Label, int* Data, float Speed = 1.0f, int Min = 0, int Max = 0,
        const char* Format = "%d", ImGuiSliderFlags Flags = 0);
    RUNTIME_API bool DragFloat3(const char* Label, float Data[3], float Speed = 1.0f, float Min = 0.0f, float Max = 0.0f,
        const char* Format = "%.3f", ImGuiSliderFlags Flags = 0);
    RUNTIME_API bool DragFloat4(const char* Label, float Data[4], float Speed = 1.0f, float Min = 0.0f, float Max = 0.0f,
        const char* Format = "%.3f", ImGuiSliderFlags Flags = 0);
}
