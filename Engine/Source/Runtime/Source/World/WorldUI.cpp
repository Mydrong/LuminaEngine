#include "UI/UILibrary.h"
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

    FVector2 CWorld::GetUILayoutSize()
    {
        const FUIntVector2 Size = RmlUi::GetWorldLayoutSize(this);
        return FVector2((float)Size.x, (float)Size.y);
    }

    bool CWorld::IsCursorOverUI()
    {
        return RmlUi::IsCursorOverWorldUI(this);
    }

    SUIDocument CWorld::LoadUIDocument(const FString& Path)
    {
        return FromPtr<SUIDocument>(RmlUi::LoadScreenDocument(this, Path));
    }

    SUIDocument CWorld::LoadUIDocumentFromMemory(const FString& Rml, const FString& SourceUrl)
    {
        return FromPtr<SUIDocument>(RmlUi::LoadScreenDocumentFromMemory(this, Rml, SourceUrl));
    }

    void CWorld::UnloadUIDocument(SUIDocument Document)
    {
        RmlUi::UnloadScreenDocument(this, AsPtr(Document));
    }

    SUIEventListener CWorld::AddUIEventListener(SUIElement Element, const FString& EventType)
    {
        return FromPtr<SUIEventListener>(RmlUi::AddElementEventListener(this, AsPtr(Element), EventType));
    }

    void CWorld::RemoveUIEventListener(SUIEventListener Listener)
    {
        RmlUi::RemoveElementEventListener(this, AsPtr(Listener));
    }

    SUIDataModel CWorld::CreateUIDataModel(const FString& Name, SScriptCallback OnSet, SScriptCallback OnEvent)
    {
        return FromPtr<SUIDataModel>(RmlUi::CreateDataModel(this, Name, OnSet, OnEvent));
    }
}
