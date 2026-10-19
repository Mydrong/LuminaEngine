#include "MaterialNode_SceneData.h"

#include "Core/Object/Cast.h"
#include "UI/Tools/NodeGraph/Material/MaterialInput.h"
#include "UI/Tools/NodeGraph/Material/MaterialOutput.h"
#include "UI/Tools/NodeGraph/Material/MaterialCompiler.h"

namespace Lumina
{
    void CMaterialExpression_ScreenPosition::BuildNode()    { Super::BuildNode(); Output->SetInputType(EMaterialInputType::Float2); Output->SetComponentMask(EComponentMask::RG); }
    void CMaterialExpression_ScreenPosition::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "ScreenPosition")) return;
        C.ScreenPosition(FullName, bRaw);
    }

    void CMaterialExpression_ViewDirection::BuildNode()     { Super::BuildNode(); Output->SetInputType(EMaterialInputType::Float3); Output->SetComponentMask(EComponentMask::RGB); }
    void CMaterialExpression_ViewDirection::GenerateDefinition(FMaterialCompiler& C) { C.ViewDirection(FullName, this); }

    void CMaterialExpression_ReflectionVector::BuildNode()  { Super::BuildNode(); Output->SetInputType(EMaterialInputType::Float3); Output->SetComponentMask(EComponentMask::RGB); }
    void CMaterialExpression_ReflectionVector::GenerateDefinition(FMaterialCompiler& C) { C.ReflectionVector(FullName, this); }

    void CMaterialExpression_FragmentDepth::BuildNode()     { Super::BuildNode(); Output->SetInputType(EMaterialInputType::Float); }
    void CMaterialExpression_FragmentDepth::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "FragmentDepth")) return;
        C.FragmentDepth(FullName, bLinear, this);
    }

    void CMaterialExpression_ViewportSize::BuildNode()      { Super::BuildNode(); Output->SetInputType(EMaterialInputType::Float2); Output->SetComponentMask(EComponentMask::RG); }
    void CMaterialExpression_ViewportSize::GenerateDefinition(FMaterialCompiler& C) { C.ViewportSize(FullName); }

    void CMaterialExpression_AspectRatio::BuildNode()       { Super::BuildNode(); Output->SetInputType(EMaterialInputType::Float); }
    void CMaterialExpression_AspectRatio::GenerateDefinition(FMaterialCompiler& C) { C.AspectRatio(FullName); }

    void CMaterialExpression_SceneColor::BuildNode()
    {
        Super::BuildNode();
        Output->SetInputType(EMaterialInputType::Float4);
        Output->SetComponentMask(EComponentMask::RGBA);

        UV = Cast<CMaterialInput>(CreatePin(CMaterialInput::StaticClass(), "UV", ENodePinDirection::Input));
        UV->SetHideDuringConnection(false);
        UV->SetPinName("UV");
    }
    void CMaterialExpression_SceneColor::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "SceneColor")) return;
        C.SceneColor(FullName, UV);
    }

    void CMaterialExpression_SceneDepth::BuildNode()
    {
        Super::BuildNode();
        Output->SetInputType(EMaterialInputType::Float);
        Output->SetComponentMask(EComponentMask::R);

        UV = Cast<CMaterialInput>(CreatePin(CMaterialInput::StaticClass(), "UV", ENodePinDirection::Input));
        UV->SetHideDuringConnection(false);
        UV->SetPinName("UV");
    }
    void CMaterialExpression_SceneDepth::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "SceneDepth")) return;
        C.SceneDepth(FullName, UV, bLinear);
    }

    void CMaterialExpression_CustomDepth::BuildNode()
    {
        Super::BuildNode();
        Output->SetInputType(EMaterialInputType::Float);
        Output->SetComponentMask(EComponentMask::R);

        UV = Cast<CMaterialInput>(CreatePin(CMaterialInput::StaticClass(), "UV", ENodePinDirection::Input));
        UV->SetHideDuringConnection(false);
        UV->SetPinName("UV");
    }
    void CMaterialExpression_CustomDepth::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "CustomDepth")) return;
        C.CustomDepth(FullName, UV, bLinear);
    }

    void CMaterialExpression_CustomStencil::BuildNode()
    {
        Super::BuildNode();
        Output->SetInputType(EMaterialInputType::Float);
        Output->SetComponentMask(EComponentMask::R);

        UV = Cast<CMaterialInput>(CreatePin(CMaterialInput::StaticClass(), "UV", ENodePinDirection::Input));
        UV->SetHideDuringConnection(false);
        UV->SetPinName("UV");
    }
    void CMaterialExpression_CustomStencil::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "CustomStencil")) return;
        C.CustomStencil(FullName, UV);
    }

    void CMaterialExpression_SceneHDRColor::BuildNode()
    {
        Super::BuildNode();
        Output->SetInputType(EMaterialInputType::Float4);
        Output->SetComponentMask(EComponentMask::RGBA);

        UV = Cast<CMaterialInput>(CreatePin(CMaterialInput::StaticClass(), "UV", ENodePinDirection::Input));
        UV->SetHideDuringConnection(false);
        UV->SetPinName("UV");
    }
    void CMaterialExpression_SceneHDRColor::GenerateDefinition(FMaterialCompiler& C)
    {
        if (!C.RequirePixelStage(this, "SceneHDRColor")) return;
        C.SceneHDRColor(FullName, UV);
    }
}
