#include "RuntimePCH.h"

#include "Assets/AssetTypes/Animation/AnimationGraph/AnimationGraph.h"
#include "Assets/AssetTypes/Animation/Montage/AnimationMontage.h"
#include "Core/Object/Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "World/Entity/Components/AnimationGraphComponent.h"
#include "World/Entity/Components/SimpleAnimationComponent.h"
#include "World/World.h"

namespace Lumina
{
    namespace
    {
        // Parameters live in the graph component's own struct instance, so a name resolves to a field on it.
        void* ResolveParameterField(CWorld& World, ECS::FEntity Entity, const FName& Name,
            EAnimParamValueType& OutType)
        {
            OutType = EAnimParamValueType::Unresolved;

            SAnimationGraphComponent* Comp = World.TryGetComponent<SAnimationGraphComponent>(Entity);
            if (Comp == nullptr || !Comp->Graph.IsValid())
            {
                return nullptr;
            }

            CStruct* Struct = Comp->Graph->GetParameterStruct();
            uint8* Base = static_cast<uint8*>(Comp->GetParameterMemory());
            if (Struct == nullptr || Base == nullptr)
            {
                return nullptr;
            }

            FProperty* Property = Struct->GetProperty(Name);
            if (Property == nullptr || Property->HasSetterOrGetter())
            {
                return nullptr;
            }

            OutType = AnimParamValueTypeFromProperty(Property);
            return OutType == EAnimParamValueType::Unresolved ? nullptr : Base + Property->Offset;
        }

        void WriteParameterScalar(void* Field, EAnimParamValueType Type, float Value)
        {
            switch (Type)
            {
            case EAnimParamValueType::Float:  *static_cast<float*>(Field)  = Value; break;
            case EAnimParamValueType::Double: *static_cast<double*>(Field) = (double)Value; break;
            case EAnimParamValueType::Bool:   *static_cast<bool*>(Field)   = Value != 0.0f; break;
            case EAnimParamValueType::Int32:  *static_cast<int32*>(Field)  = (int32)Value; break;
            case EAnimParamValueType::UInt32: *static_cast<uint32*>(Field) = (uint32)Value; break;
            case EAnimParamValueType::Int64:  *static_cast<int64*>(Field)  = (int64)Value; break;
            case EAnimParamValueType::UInt64: *static_cast<uint64*>(Field) = (uint64)Value; break;
            case EAnimParamValueType::Int16:  *static_cast<int16*>(Field)  = (int16)Value; break;
            case EAnimParamValueType::UInt16: *static_cast<uint16*>(Field) = (uint16)Value; break;
            case EAnimParamValueType::Int8:   *static_cast<int8*>(Field)   = (int8)Value; break;
            case EAnimParamValueType::UInt8:  *static_cast<uint8*>(Field)  = (uint8)Value; break;
            default: break;
            }
        }

        SSimpleAnimationComponent* SimpleOf(CWorld& World, ECS::FEntity Entity)
        {
            return World.TryGetComponent<SSimpleAnimationComponent>(Entity);
        }

        SAnimationGraphComponent* GraphOf(CWorld& World, ECS::FEntity Entity)
        {
            return World.TryGetComponent<SAnimationGraphComponent>(Entity);
        }
    }

    void CWorld::PlayAnimation(ECS::FEntity Entity, CAnimation* Clip, bool bLoop, float Speed)
    {
        SSimpleAnimationComponent& Comp = GetOrEmplaceComponent<SSimpleAnimationComponent>(Entity);
        Comp.PlayAnimation(Clip, bLoop, Speed);
    }

    void CWorld::StopAnimation(ECS::FEntity Entity)
    {
        if (SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity)) { Comp->Stop(); }
    }

    void CWorld::PauseAnimation(ECS::FEntity Entity)
    {
        if (SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity)) { Comp->Pause(); }
    }

    void CWorld::ResumeAnimation(ECS::FEntity Entity)
    {
        if (SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity)) { Comp->Resume(); }
    }

    bool CWorld::IsAnimationPlaying(ECS::FEntity Entity)
    {
        const SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity);
        return Comp != nullptr && Comp->IsPlaying();
    }

    bool CWorld::IsAnimationFinished(ECS::FEntity Entity)
    {
        const SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity);
        return Comp != nullptr && Comp->IsFinished();
    }

    void CWorld::SetAnimationSpeed(ECS::FEntity Entity, float Speed)
    {
        if (SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity)) { Comp->PlaybackSpeed = Speed; }
    }

    void CWorld::SetAnimationTime(ECS::FEntity Entity, float Time)
    {
        if (SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity))
        {
            Comp->CurrentTime = Time;
            Comp->bDirty = true;
        }
    }

    float CWorld::GetAnimationTime(ECS::FEntity Entity)
    {
        const SSimpleAnimationComponent* Comp = SimpleOf(*this, Entity);
        return Comp != nullptr ? Comp->CurrentTime : 0.0f;
    }

    void CWorld::SetAnimFloat(ECS::FEntity Entity, const FName& Name, float Value)
    {
        EAnimParamValueType Type;
        if (void* Field = ResolveParameterField(*this, Entity, Name, Type))
        {
            WriteParameterScalar(Field, Type, Value);
        }
    }

    float CWorld::GetAnimFloat(ECS::FEntity Entity, const FName& Name, float Default)
    {
        EAnimParamValueType Type;
        void* Field = ResolveParameterField(*this, Entity, Name, Type);
        if (Field == nullptr)
        {
            return Default;
        }

        FAnimGraphParamBinding Binding;
        Binding.Offset = 0;
        Binding.Type = Type;
        return ReadAnimParamScalar(static_cast<const uint8*>(Field), Binding);
    }

    void CWorld::SetAnimBool(ECS::FEntity Entity, const FName& Name, bool bValue)
    {
        SetAnimFloat(Entity, Name, bValue ? 1.0f : 0.0f);
    }

    bool CWorld::GetAnimBool(ECS::FEntity Entity, const FName& Name, bool bDefault)
    {
        return GetAnimFloat(Entity, Name, bDefault ? 1.0f : 0.0f) != 0.0f;
    }

    bool CWorld::HasAnimParameter(ECS::FEntity Entity, const FName& Name)
    {
        const SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        return Comp != nullptr && Comp->HasParameter(Name);
    }

    uint32 CWorld::PlayMontage(ECS::FEntity Entity, CAnimationMontage* Montage,
        float PlayRate, const FName& Section)
    {
        if (Montage == nullptr)
        {
            return 0;
        }
        SAnimationGraphComponent& Comp = GetOrEmplaceComponent<SAnimationGraphComponent>(Entity);
        return Comp.Montages.Play(Montage, PlayRate, Section);
    }

    void CWorld::StopMontage(ECS::FEntity Entity, CAnimationMontage* Montage,
        float BlendOutTime)
    {
        SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        if (Comp == nullptr)
        {
            return;
        }
        if (Montage != nullptr)
        {
            Comp->Montages.Stop(Montage, BlendOutTime);
        }
        else
        {
            Comp->Montages.StopAll(BlendOutTime);
        }
    }

    bool CWorld::JumpToMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage,
        const FName& Section)
    {
        SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        return Comp != nullptr && Montage != nullptr && Comp->Montages.JumpToSection(Montage, Section);
    }

    bool CWorld::SetNextMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage,
        const FName& Section)
    {
        SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        return Comp != nullptr && Montage != nullptr && Comp->Montages.SetNextSection(Montage, Section);
    }

    bool CWorld::IsMontagePlaying(ECS::FEntity Entity, CAnimationMontage* Montage)
    {
        const SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        if (Comp == nullptr)
        {
            return false;
        }
        return Montage != nullptr ? Comp->Montages.IsPlaying(Montage) : Comp->Montages.HasActive();
    }

    float CWorld::GetMontagePosition(ECS::FEntity Entity, CAnimationMontage* Montage)
    {
        const SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        return (Comp != nullptr && Montage != nullptr) ? Comp->Montages.GetPosition(Montage) : 0.0f;
    }

    float CWorld::GetMontageWeight(ECS::FEntity Entity, CAnimationMontage* Montage)
    {
        const SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        return (Comp != nullptr && Montage != nullptr) ? Comp->Montages.GetWeight(Montage) : 0.0f;
    }

    void CWorld::SetMontagePlayRate(ECS::FEntity Entity, CAnimationMontage* Montage,
        float PlayRate)
    {
        SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        if (Comp != nullptr && Montage != nullptr)
        {
            Comp->Montages.SetPlayRate(Montage, PlayRate);
        }
    }

    FName CWorld::GetMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage)
    {
        const SAnimationGraphComponent* Comp = GraphOf(*this, Entity);
        return (Comp != nullptr && Montage != nullptr) ? Comp->Montages.GetCurrentSection(Montage) : FName();
    }
}
