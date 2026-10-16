#include "RuntimePCH.h"
#include "PropertyRegistry.h"

#include "LuminaTypes.h"
#include "Core/Reflection/PropertyTypeTraits.h"
#include "Core/Object/Class.h"
#include "Core/Object/InstancedStruct.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Core/Object/SoftObjectPtr.h"
#include "World/Entity/Components/ComponentRef.h"
#include "Properties/ArrayProperty.h"
#include "Properties/ClassProperty.h"
#include "Properties/ComponentRefProperty.h"
#include "Properties/DelegateProperty.h"
#include "Properties/EnumProperty.h"
#include "Properties/InstancedStructProperty.h"
#include "Properties/MapProperty.h"
#include "Properties/ObjectProperty.h"
#include "Properties/OptionalProperty.h"
#include "Properties/SoftObjectProperty.h"
#include "Properties/StringProperty.h"
#include "Properties/StructProperty.h"
#include "Properties/SubStructProperty.h"

namespace Lumina
{
    namespace
    {
        template<typename TPropertyType>
        FProperty* ConstructKind(const FPropertyOwner& Owner, const FPropertyDesc& Desc)
        {
            FProperty* Property = Owner.Build<TPropertyType>(Desc);

            // A drift between the table's footprint and the constructor's would mis-plan a script layout.
            DEBUG_ASSERT(GetPropertyKindOps(Desc.Type->Kind).Size == 0
                || GetPropertyKindOps(Desc.Type->Kind).Size == Property->GetElementSize());

            return Property;
        }

        struct FPropertyKindTable
        {
            FPropertyKindOps Entries[(size_t)EPropertyTypeFlags::Count];
            FPropertyKindOps Unregistered;

            template<typename TPropertyType>
            void Add(EPropertyTypeFlags Kind)
            {
                Entries[(size_t)Kind].Construct = &ConstructKind<TPropertyType>;
            }

            // A kind whose footprint is the C++ type it stores, and which names nothing.
            template<typename TPropertyType, typename TCppType>
            void AddBasic(EPropertyTypeFlags Kind)
            {
                FPropertyKindOps& Entry = Entries[(size_t)Kind];
                Entry.Construct   = &ConstructKind<TPropertyType>;
                Entry.Size        = sizeof(TCppType);
                Entry.Alignment   = alignof(TCppType);
                Entry.bArithmetic = std::is_arithmetic_v<TCppType> || std::is_same_v<TCppType, ECS::FEntity>;
                Entry.BasicType   = PropertyTypeOf<TCppType>;
            }

            template<typename TPropertyType, typename TCppType>
            void AddSized(EPropertyTypeFlags Kind)
            {
                Add<TPropertyType>(Kind);
                Entries[(size_t)Kind].Size      = sizeof(TCppType);
                Entries[(size_t)Kind].Alignment = alignof(TCppType);
            }

            FPropertyKindTable()
            {
                AddBasic<FBoolProperty,   bool>        (EPropertyTypeFlags::Bool);
                AddBasic<FInt8Property,   int8>        (EPropertyTypeFlags::Int8);
                AddBasic<FInt16Property,  int16>       (EPropertyTypeFlags::Int16);
                AddBasic<FInt32Property,  int32>       (EPropertyTypeFlags::Int32);
                AddBasic<FInt64Property,  int64>       (EPropertyTypeFlags::Int64);
                AddBasic<FUInt8Property,  uint8>       (EPropertyTypeFlags::UInt8);
                AddBasic<FUInt16Property, uint16>      (EPropertyTypeFlags::UInt16);
                AddBasic<FUInt32Property, uint32>      (EPropertyTypeFlags::UInt32);
                AddBasic<FUInt64Property, uint64>      (EPropertyTypeFlags::UInt64);
                AddBasic<FEntityProperty, ECS::FEntity>(EPropertyTypeFlags::Entity);
                AddBasic<FFloatProperty,  float>       (EPropertyTypeFlags::Float);
                AddBasic<FDoubleProperty, double>      (EPropertyTypeFlags::Double);
                AddBasic<FStringProperty, FString>     (EPropertyTypeFlags::String);
                AddBasic<FNameProperty,   FName>       (EPropertyTypeFlags::Name);

                AddSized<FObjectProperty,          TStrongObjectPtr<CObject>>(EPropertyTypeFlags::Object);
                AddSized<FSoftObjectProperty,      FSoftObjectPath>          (EPropertyTypeFlags::SoftObject);
                AddSized<FClassProperty,           void*>                    (EPropertyTypeFlags::Class);
                AddSized<FSubStructProperty,       void*>                    (EPropertyTypeFlags::SubStruct);
                AddSized<FDelegateProperty,        FScriptDelegate>          (EPropertyTypeFlags::Delegate);
                AddSized<FInstancedStructProperty, FInstancedStruct>         (EPropertyTypeFlags::InstancedStruct);
                AddSized<FComponentRefProperty,    FComponentRef>            (EPropertyTypeFlags::ComponentRef);

                // These are as wide as what they resolve to, which is only known at registration.
                Add<FStructProperty>  (EPropertyTypeFlags::Struct);
                Add<FEnumProperty>    (EPropertyTypeFlags::Enum);
                Add<FArrayProperty>   (EPropertyTypeFlags::Vector);
                Add<FOptionalProperty>(EPropertyTypeFlags::Optional);
                Add<FMapProperty>     (EPropertyTypeFlags::Map);
            }
        };
    }

    const FPropertyKindOps& GetPropertyKindOps(EPropertyTypeFlags Kind)
    {
        static const FPropertyKindTable Table;

        const size_t Index = (size_t)Kind;
        if (Index >= (size_t)EPropertyTypeFlags::Count)
        {
            return Table.Unregistered;
        }
        return Table.Entries[Index];
    }
}
