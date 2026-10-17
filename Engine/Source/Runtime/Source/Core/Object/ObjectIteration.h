#pragma once

#include "Containers/FunctionRef.h"
#include "Containers/Invoke.h"

namespace Lumina
{
    class CObjectBase;
}

namespace Lumina
{
    RUNTIME_API void ForEachObject(TFunctionRef<bool(CObjectBase*, int32)> Function);

    template<typename Func>
    requires std::is_same_v<std::invoke_result_t<Func, CObjectBase*, int32>, void>
    void ForEachObject(Func&& Function)
    {
        auto Continue = [&](CObjectBase* Object, int32 Index)
        {
            Invoke(Function, Object, Index);
            return true;
        };
        ForEachObject(TFunctionRef<bool(CObjectBase*, int32)>(Continue));
    }
}
