#pragma once

#include "Core/Math/Easing.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Scripting/ScriptCallback.h"
#include "World/ECS/Entity.h"
#include "TweenLibrary.generated.h"

namespace Lumina
{
    class CWorld;

    /** Building a tween a step at a time. Every call takes the id Create returned; a stale one is a no-op. */
    REFLECT()
    class RUNTIME_API CTweenLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        /** Starts an empty tween and returns its id. Owner ties its lifetime to that entity. */
        FUNCTION()
        static uint32 Create(CWorld* World, ECS::FEntity Owner);

        FUNCTION()
        static void MoveTo(CWorld* World, uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration);

        FUNCTION()
        static void RotateTo(CWorld* World, uint32 Tween, ECS::FEntity Target, FQuat Destination, float Duration);

        FUNCTION()
        static void ScaleTo(CWorld* World, uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration);

        /** Interpolates a bare number, reporting each value to OnValue until the tween is destroyed. */
        FUNCTION()
        static void ValueTo(CWorld* World, uint32 Tween, float From, float To, float Duration, SScriptCallback OnValue);

        /** Dead time, for spacing steps apart. */
        FUNCTION()
        static void Interval(CWorld* World, uint32 Tween, float Duration);

        /** Runs OnCall when the sequence reaches this step. */
        FUNCTION()
        static void Call(CWorld* World, uint32 Tween, SScriptCallback OnCall);

        /** Runs OnFinished after the last step, including after the final loop. */
        FUNCTION()
        static void OnFinished(CWorld* World, uint32 Tween, SScriptCallback OnFinished);

        /** Transition, Ease and Delay all apply to the step that was added last. */
        FUNCTION()
        static void Trans(CWorld* World, uint32 Tween, EEaseTransition Transition);

        FUNCTION()
        static void Ease(CWorld* World, uint32 Tween, EEaseType Ease);

        FUNCTION()
        static void Delay(CWorld* World, uint32 Tween, float Seconds);

        /** Runs the last step alongside the one before it rather than after it. */
        FUNCTION()
        static void Parallel(CWorld* World, uint32 Tween);

        /** Zero or less loops forever. */
        FUNCTION()
        static void SetLoops(CWorld* World, uint32 Tween, int32 Count);

        FUNCTION()
        static void SetSpeedScale(CWorld* World, uint32 Tween, float Scale);

        FUNCTION()
        static void SetPaused(CWorld* World, uint32 Tween, bool bPaused);

        /** Stops where it is; whatever it was driving keeps its current value. */
        FUNCTION()
        static void Kill(CWorld* World, uint32 Tween);

        FUNCTION()
        static bool IsRunning(CWorld* World, uint32 Tween);
    };
}
