#pragma once

#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "Containers/String.h"
#include "Scripting/ScriptCallback.h"
#include "UI/UITypes.h"
#include "UILibrary.generated.h"

namespace Lumina
{
    class CWorld;
    class CEntityScript;

    // The script-facing half of the RmlUi bridge, reflected so C# binds it instead of hand-written thunks.
    REFLECT()
    class RUNTIME_API CUILibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** UI pixel size of the world's screen context, which in the editor is the panel, not the target. */
        FUNCTION()
        static FVector2 GetLayoutSize(CWorld* World);

        /** True while the cursor is on an interface element, so a click to move can ignore that click. */
        FUNCTION()
        static bool IsCursorOverUI(CWorld* World);

        //~ UI scripts, callable on any CUIScript from either language

        FUNCTION()
        static void ShowScriptUI(CEntityScript* Script);

        FUNCTION()
        static void HideScriptUI(CEntityScript* Script);

        FUNCTION()
        static bool IsScriptUIShown(CEntityScript* Script);

        FUNCTION()
        static SUIDocument GetScriptDocument(CEntityScript* Script);

        //~ Documents

        /** Loads a virtual path into the world's screen context. The document starts hidden. */
        FUNCTION()
        static SUIDocument LoadDocument(CWorld* World, const FString& Path);

        /** SourceUrl resolves the document's relative includes such as stylesheets and images. */
        FUNCTION()
        static SUIDocument LoadDocumentFromMemory(CWorld* World, const FString& Rml, const FString& SourceUrl);

        FUNCTION()
        static void UnloadDocument(CWorld* World, SUIDocument Document);

        /** Modal blocks focus to other documents, AutoFocus focuses the document's first autofocus element. */
        FUNCTION()
        static void ShowDocument(SUIDocument Document, bool bModal = false, bool bAutoFocus = true);

        FUNCTION()
        static void HideDocument(SUIDocument Document);

        FUNCTION()
        static void BringDocumentToFront(SUIDocument Document);

        /** The document's body element. */
        FUNCTION()
        static SUIElement GetDocumentRoot(SUIDocument Document);

        FUNCTION()
        static SUIElement GetElementById(SUIDocument Document, const FString& Id);

        //~ Elements

        FUNCTION()
        static SUIElement QuerySelector(SUIElement Element, const FString& Selector);

        FUNCTION()
        static void SetInnerRml(SUIElement Element, const FString& Rml);

        FUNCTION()
        static FString GetInnerRml(SUIElement Element);

        FUNCTION()
        static void SetAttribute(SUIElement Element, const FString& Name, const FString& Value);

        FUNCTION()
        static FString GetAttribute(SUIElement Element, const FString& Name);

        /** Sets an inline CSS property. */
        FUNCTION()
        static void SetProperty(SUIElement Element, const FString& Name, const FString& Value);

        FUNCTION()
        static void RemoveProperty(SUIElement Element, const FString& Name);

        FUNCTION()
        static void SetClass(SUIElement Element, const FString& Class, bool bActive);

        FUNCTION()
        static bool IsClassSet(SUIElement Element, const FString& Class);

        FUNCTION()
        static void FocusElement(SUIElement Element);

        FUNCTION()
        static void BlurElement(SUIElement Element);

        FUNCTION()
        static void ClickElement(SUIElement Element);

        /** Border box in document pixels as X, Y, Width, Height. Zeroes when the element is gone. */
        FUNCTION()
        static FVector4 GetElementBox(SUIElement Element);

        //~ Event listeners

        FUNCTION()
        static SUIEventListener AddEventListener(CWorld* World, SUIElement Element, const FString& EventType);

        FUNCTION()
        static void RemoveEventListener(CWorld* World, SUIEventListener Listener);

        /** The listener's FScriptDelegateBase, which script binds its handler to. */
        FUNCTION()
        static uint64 GetEventListenerDelegate(SUIEventListener Listener);

        //~ Data models

        // OnSet gets an FUIDataSet and OnEvent an FUIDataEvent by address, and the model owns both from here.
        FUNCTION(ScriptInternal)
        static SUIDataModel CreateDataModel(CWorld* World, const FString& Name, SScriptCallback OnSet, SScriptCallback OnEvent);

        FUNCTION()
        static void DestroyDataModel(SUIDataModel Model);

        /** Registers a scalar variable and returns its field id, or -1 on failure. */
        FUNCTION()
        static int32 BindScalar(SUIDataModel Model, const FString& Name, EUIVarType Type);

        /** Registers a callback bound by data-event-*, dispatched back under CommandId. */
        FUNCTION()
        static void BindCommand(SUIDataModel Model, const FString& Name, int32 CommandId);

        FUNCTION()
        static void SetNumber(SUIDataModel Model, int32 Field, double Value);

        FUNCTION()
        static void SetString(SUIDataModel Model, int32 Field, const FString& Value);

        /** Marks a variable dirty so RmlUi refreshes its views on the next context update. */
        FUNCTION()
        static void MarkDirty(SUIDataModel Model, int32 Field);

        FUNCTION()
        static void MarkAllDirty(SUIDataModel Model);

        //~ Data model lists, the array-of-struct variables data-for iterates

        /** Returns a list field id in its own index space, separate from the scalar fields, or -1. */
        FUNCTION()
        static int32 BindList(SUIDataModel Model, const FString& Name);

        /** Adds a named column reachable in RML as {{ item.Name }} and returns its index. */
        FUNCTION()
        static int32 BindListMember(SUIDataModel Model, int32 ListField, const FString& MemberName);

        /** Clears the list to RowCount empty rows. Set every cell next, then mark it dirty. */
        FUNCTION()
        static void ResizeList(SUIDataModel Model, int32 ListField, int32 RowCount);

        FUNCTION()
        static void SetListCell(SUIDataModel Model, int32 ListField, int32 Row, int32 Col, const FString& Value);

        FUNCTION()
        static void MarkListDirty(SUIDataModel Model, int32 ListField);
    };
}
