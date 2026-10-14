#include "RuntimePCH.h"
#include "GameplayMessageLibrary.h"

#include "Core/Object/Class.h"
#include "GameplayTags/GameplayMessageBus.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // The context is the listener's callback token, and the message crosses as its address.
        uint32 InvokeManagedListener(void* Context, const FGameplayMessage* Message)
        {
            return Scripting::InvokeScriptCallbackRepeating(SScriptCallback{ reinterpret_cast<uint64>(Context) }, reinterpret_cast<uint64>(Message)) != 0 ? 1u : 0u;
        }
    }

    uint64 CGameplayMessageLibrary::GetMessageTypeKey(FName TypeName)
    {
        return TypeName.IsNone() ? 0 : MakeGameplayMessageType(TypeName);
    }

    uint64 CGameplayMessageLibrary::Subscribe(CWorld* World, uint32 Channel, ECS::FEntity Owner, uint64 Type, bool bExactMatch, SScriptCallback Listener)
    {
        if (World == nullptr || !Listener.IsBound())
        {
            Scripting::ReleaseScriptCallback(Listener);
            return 0;
        }

        FGameplayMessageBus::FSubscribeParams Params;
        Params.Channel  = Channel;
        Params.Owner    = Owner;
        Params.Type     = Type;
        Params.Match    = bExactMatch ? EGameplayTagMatch::Exact : EGameplayTagMatch::Partial;
        Params.Invoke   = &InvokeManagedListener;
        Params.Context  = reinterpret_cast<void*>(Listener.Token);
        Params.Destroy  = &Scripting::FreeManagedHandle;
        Params.bManaged = true;
        return World->GetMessageBus().Subscribe(Params);
    }

    bool CGameplayMessageLibrary::Unsubscribe(CWorld* World, uint64 Id)
    {
        return World != nullptr && World->GetMessageBus().Unsubscribe(Id);
    }

    void CGameplayMessageLibrary::UnsubscribeAll(CWorld* World, ECS::FEntity Owner)
    {
        if (World != nullptr)
        {
            World->GetMessageBus().UnsubscribeAll(Owner);
        }
    }

    void CGameplayMessageLibrary::Send(CWorld* World, uint32 Route, ECS::FEntity Target, uint32 Channel, uint32 Kind, uint64 Type,
        void* Data, uint32 Size, void* Struct, bool bIncludeSelf)
    {
        FGameplayMessagePayloadRef Payload;
        Payload.Type   = Type;
        Payload.Data   = Data;
        Payload.Size   = Size;
        Payload.Kind   = (EGameplayMessagePayload)Kind;
        Payload.Struct = static_cast<const CStruct*>(Struct);

        if (World == nullptr || Route > (uint32)EGameplayMessageRoute::To)
        {
            if (Payload.Kind == EGameplayMessagePayload::Managed)
            {
                Scripting::FreeManagedHandle(Data);
            }
            return;
        }
        World->GetMessageBus().Send((EGameplayMessageRoute)Route, Target, Channel, Payload, bIncludeSelf);
    }
}
