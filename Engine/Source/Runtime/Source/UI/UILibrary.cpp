#include "UI/UILibrary.h"
#include "UI/UIScript.h"

#include "UI/RmlUiBridge.h"

namespace Lumina
{
    namespace
    {
        template<typename THandle>
        void* AsPtr(THandle Handle)
        {
            return reinterpret_cast<void*>(Handle.Handle);
        }

        template<typename THandle>
        THandle FromPtr(void* Ptr)
        {
            THandle Handle;
            Handle.Handle = reinterpret_cast<uint64>(Ptr);
            return Handle;
        }
    }

    void CUILibrary::ShowScriptUI(CEntityScript* Script)
    {
        UIScripts::Show(Script);
    }

    void CUILibrary::HideScriptUI(CEntityScript* Script)
    {
        UIScripts::Hide(Script);
    }

    bool CUILibrary::IsScriptUIShown(CEntityScript* Script)
    {
        return UIScripts::IsShown(Script);
    }

    SUIDocument CUILibrary::GetScriptDocument(CEntityScript* Script)
    {
        return FromPtr<SUIDocument>(UIScripts::GetDocument(Script));
    }

    void CUILibrary::ShowDocument(SUIDocument Document, bool bModal, bool bAutoFocus)
    {
        RmlUi::ShowDocument(AsPtr(Document), bModal, bAutoFocus);
    }

    void CUILibrary::HideDocument(SUIDocument Document)
    {
        RmlUi::HideDocument(AsPtr(Document));
    }

    void CUILibrary::BringDocumentToFront(SUIDocument Document)
    {
        RmlUi::PullDocumentToFront(AsPtr(Document));
    }

    SUIElement CUILibrary::GetDocumentRoot(SUIDocument Document)
    {
        return FromPtr<SUIElement>(RmlUi::GetDocumentRoot(AsPtr(Document)));
    }

    SUIElement CUILibrary::GetElementById(SUIDocument Document, const FString& Id)
    {
        return FromPtr<SUIElement>(RmlUi::DocumentGetElementById(AsPtr(Document), Id));
    }

    SUIElement CUILibrary::QuerySelector(SUIElement Element, const FString& Selector)
    {
        return FromPtr<SUIElement>(RmlUi::ElementQuerySelector(AsPtr(Element), Selector));
    }

    void CUILibrary::SetInnerRml(SUIElement Element, const FString& Rml)
    {
        RmlUi::ElementSetInnerRml(AsPtr(Element), Rml);
    }

    FString CUILibrary::GetInnerRml(SUIElement Element)
    {
        return RmlUi::ElementGetInnerRml(AsPtr(Element));
    }

    void CUILibrary::SetAttribute(SUIElement Element, const FString& Name, const FString& Value)
    {
        RmlUi::ElementSetAttribute(AsPtr(Element), Name, Value);
    }

    FString CUILibrary::GetAttribute(SUIElement Element, const FString& Name)
    {
        return RmlUi::ElementGetAttribute(AsPtr(Element), Name);
    }

    void CUILibrary::SetProperty(SUIElement Element, const FString& Name, const FString& Value)
    {
        RmlUi::ElementSetProperty(AsPtr(Element), Name, Value);
    }

    void CUILibrary::RemoveProperty(SUIElement Element, const FString& Name)
    {
        RmlUi::ElementRemoveProperty(AsPtr(Element), Name);
    }

    void CUILibrary::SetClass(SUIElement Element, const FString& Class, bool bActive)
    {
        RmlUi::ElementSetClass(AsPtr(Element), Class, bActive);
    }

    bool CUILibrary::IsClassSet(SUIElement Element, const FString& Class)
    {
        return RmlUi::ElementIsClassSet(AsPtr(Element), Class);
    }

    void CUILibrary::FocusElement(SUIElement Element)
    {
        RmlUi::ElementFocus(AsPtr(Element));
    }

    void CUILibrary::BlurElement(SUIElement Element)
    {
        RmlUi::ElementBlur(AsPtr(Element));
    }

    void CUILibrary::ClickElement(SUIElement Element)
    {
        RmlUi::ElementClick(AsPtr(Element));
    }

    FVector4 CUILibrary::GetElementBox(SUIElement Element)
    {
        float XYWH[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        RmlUi::ElementGetBox(AsPtr(Element), XYWH);
        return FVector4(XYWH[0], XYWH[1], XYWH[2], XYWH[3]);
    }

    uint64 CUILibrary::GetEventListenerDelegate(SUIEventListener Listener)
    {
        return reinterpret_cast<uint64>(RmlUi::GetElementEventListenerDelegate(AsPtr(Listener)));
    }

    void CUILibrary::DestroyDataModel(SUIDataModel Model)
    {
        RmlUi::DestroyDataModel(AsPtr(Model));
    }

    int32 CUILibrary::BindScalar(SUIDataModel Model, const FString& Name, EUIVarType Type)
    {
        return RmlUi::DataModelBindScalar(AsPtr(Model), Name, (int32)Type);
    }

    void CUILibrary::BindCommand(SUIDataModel Model, const FString& Name, int32 CommandId)
    {
        RmlUi::DataModelBindCommand(AsPtr(Model), Name, CommandId);
    }

    void CUILibrary::SetNumber(SUIDataModel Model, int32 Field, double Value)
    {
        RmlUi::DataModelSetNumber(AsPtr(Model), Field, Value);
    }

    void CUILibrary::SetString(SUIDataModel Model, int32 Field, const FString& Value)
    {
        RmlUi::DataModelSetString(AsPtr(Model), Field, Value);
    }

    void CUILibrary::MarkDirty(SUIDataModel Model, int32 Field)
    {
        RmlUi::DataModelDirty(AsPtr(Model), Field);
    }

    void CUILibrary::MarkAllDirty(SUIDataModel Model)
    {
        RmlUi::DataModelDirtyAll(AsPtr(Model));
    }

    int32 CUILibrary::BindList(SUIDataModel Model, const FString& Name)
    {
        return RmlUi::DataModelBindList(AsPtr(Model), Name);
    }

    int32 CUILibrary::BindListMember(SUIDataModel Model, int32 ListField, const FString& MemberName)
    {
        return RmlUi::DataModelBindListMember(AsPtr(Model), ListField, MemberName);
    }

    void CUILibrary::ResizeList(SUIDataModel Model, int32 ListField, int32 RowCount)
    {
        RmlUi::DataModelListResize(AsPtr(Model), ListField, RowCount);
    }

    void CUILibrary::SetListCell(SUIDataModel Model, int32 ListField, int32 Row, int32 Col, const FString& Value)
    {
        RmlUi::DataModelListSetCell(AsPtr(Model), ListField, Row, Col, Value);
    }

    void CUILibrary::MarkListDirty(SUIDataModel Model, int32 ListField)
    {
        RmlUi::DataModelListDirty(AsPtr(Model), ListField);
    }
}
