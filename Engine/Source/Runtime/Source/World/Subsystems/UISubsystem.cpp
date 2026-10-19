#include "World/Subsystems/UISubsystem.h"
#include "UI/UIScript.h"

#include "UI/RmlUiBridge.h"
#include "World/World.h"

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

    FVector2 CUISubsystem::GetLayoutSize()
    {
        const FUIntVector2 Size = RmlUi::GetWorldLayoutSize(GetWorld());
        return FVector2((float)Size.x, (float)Size.y);
    }

    bool CUISubsystem::IsCursorOverUI()
    {
        return RmlUi::IsCursorOverWorldUI(GetWorld());
    }

    SUIDocument CUISubsystem::LoadDocument(const FString& Path)
    {
        return FromPtr<SUIDocument>(RmlUi::LoadScreenDocument(GetWorld(), Path));
    }

    SUIDocument CUISubsystem::LoadDocumentFromMemory(const FString& Rml, const FString& SourceUrl)
    {
        return FromPtr<SUIDocument>(RmlUi::LoadScreenDocumentFromMemory(GetWorld(), Rml, SourceUrl));
    }

    void CUISubsystem::UnloadDocument(SUIDocument Document)
    {
        RmlUi::UnloadScreenDocument(GetWorld(), AsPtr(Document));
    }

    SUIEventListener CUISubsystem::AddEventListener(SUIElement Element, const FString& EventType)
    {
        return FromPtr<SUIEventListener>(RmlUi::AddElementEventListener(GetWorld(), AsPtr(Element), EventType));
    }

    void CUISubsystem::RemoveEventListener(SUIEventListener Listener)
    {
        RmlUi::RemoveElementEventListener(GetWorld(), AsPtr(Listener));
    }

    SUIDataModel CUISubsystem::CreateDataModel(const FString& Name, SScriptCallback OnSet, SScriptCallback OnEvent)
    {
        return FromPtr<SUIDataModel>(RmlUi::CreateDataModel(GetWorld(), Name, OnSet, OnEvent));
    }
}
