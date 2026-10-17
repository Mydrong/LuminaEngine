#include "RuntimePCH.h"
#include "ObjectHandle.h"

#include "ObjectArray.h"
#include "ObjectBase.h"

namespace Lumina
{
    FObjectHandle::FObjectHandle(const CObjectBase* InObject)
    {
        if (InObject == nullptr)
        {
            *this = FObjectHandle();
            return;
        }
        *this = Object::Internal::GetHandleByIndex(InObject->GetInternalIndex());
    }

    CObject* FObjectHandle::Resolve() const
    {
        return (CObject*)Object::Internal::ResolveHandle(*this);
    }
}
