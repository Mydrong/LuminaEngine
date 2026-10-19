#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "Containers/String.h"
#include "Scripting/ScriptCallback.h"
#include "UI/UITypes.h"
#include "World/Subsystems/WorldSubsystem.h"
#include "UISubsystem.generated.h"

namespace Lumina
{
    class CEntityScript;

    // UI documents, their event listeners and data models.
    REFLECT()
    class RUNTIME_API CUISubsystem : public CWorldSubsystem
    {
        GENERATED_BODY()

    public:

        /** UI pixel size of the world's screen context, which in the editor is the panel, not the target. */
        FUNCTION()
        FVector2 GetLayoutSize();

        /** True while the cursor is on an interface element, so a click to move can ignore that click. */
        FUNCTION()
        bool IsCursorOverUI();

        //~ Documents

        /** Loads a virtual path into the world's screen context. The document starts hidden. */
        FUNCTION()
        SUIDocument LoadDocument(const FString& Path);

        /** SourceUrl resolves the document's relative includes such as stylesheets and images. */
        FUNCTION()
        SUIDocument LoadDocumentFromMemory(const FString& Rml, const FString& SourceUrl);

        FUNCTION()
        void UnloadDocument(SUIDocument Document);

        //~ Event listeners

        FUNCTION()
        SUIEventListener AddEventListener(SUIElement Element, const FString& EventType);

        FUNCTION()
        void RemoveEventListener(SUIEventListener Listener);

        //~ Data models

        // OnSet gets an FUIDataSet and OnEvent an FUIDataEvent by address, and the model owns both from here.
        FUNCTION(ScriptInternal)
        SUIDataModel CreateDataModel(const FString& Name, SScriptCallback OnSet, SScriptCallback OnEvent);
    };
}
