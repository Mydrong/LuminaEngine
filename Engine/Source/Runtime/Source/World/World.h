#pragma once

#include "Containers/BoundedQueue.h"
#include "Containers/FunctionRef.h"
#include "Containers/Span.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Object/Object.h"
#include "Core/Object/SubclassOf.h"
#include "Core/UpdateContext.h"
#include "Memory/SmartPtr.h"

#include "World/ECS/CommandBus.h"
#include "World/ECS/EventQueue.h"
#include "World/ECS/Registry.h"
#include "World/Entity/Components/CameraComponent.h"
#include "World/Entity/Systems/EntitySystem.h"
#include "World/Entity/Systems/SystemContext.h"
#include "World/Scene/RenderScene/RenderScene.h"
#include "World/Scene/RenderScene/TexturePaintTypes.h"
#include "World/Subsystems/CoroutineManager.h"
#include "World/Subsystems/TimerManager.h"
#include "World/Subsystems/TweenManager.h"
#include "World/Subsystems/WorldSubsystem.h"
#include "World/WorldContext.h"
#include "World/WorldDebugInterface.h"
#include "World/WorldTypes.h"

#include "GameplayTags/GameplayMessageBus.h"
#include "Input/InputModeLeases.h"
#include "Core/Math/AABB.h"
#include "Physics/PhysicsScene.h"
#include "Physics/PhysicsTypes.h"
#include "Physics/Ray/RayCast.h"
#include "Scripting/EntityScript.h"
#include "Scripting/ScriptCallback.h"
#include "UI/WorldUIContext.h"

#include "AI/Navigation/NavTypes.h"
#include "AI/Perception/PerceptionTypes.h"
#include "Assets/AssetTypes/ParticleSystem/ParticleSystem.h"
#include "Assets/AssetTypes/Sequence/Sequence.h"
#include "Containers/Name.h"
#include "Containers/String.h"
#include "Containers/Vector.h"
#include "Core/Math/Easing.h"
#include "Core/Math/Quat/Quat.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "Core/Object/SoftObjectPtr.h"
#include "Events/KeyCodes.h"
#include "Events/MouseCodes.h"
#include "GameplayTags/GameplayTag.h"
#include "Input/InputAction.h"
#include "Input/InputMode.h"
#include "UI/UITypes.h"
#include "World/ECS/Entity.h"
#include "World/Entity/Components/FoliageComponent.h"
#include "World/Entity/Systems/CameraSystem.h"
#include "World/Subsystems/TweenTypes.h"

#include "World.generated.h"

namespace Lumina
{
    class CAnimation;
    class CAnimationMontage;
    class CCurveAsset;
    class CEntityScript;
    class CMaterialInterface;
    class CPrefab;
    class CSaveGame;
    class CSequence;
    class CStaticMesh;
    class CParticleSystem;
    class CTexture;
    class CTextureRenderTarget;
    class FImmediateLineRenderer;
    struct FLineBatcherComponent;
    struct FResolvedSceneView;
    struct FTriangleBatcherComponent;
    struct SDefaultWorldSettings;
    struct SSceneFolderComponent;

    // One screen-space debug text line, drained by the render scene each frame.
    struct FDebugTextLine
    {
        FString  Text;
        FVector4 Color = FVector4(1.0f);
    };

    // One system as scheduled in a stage, as the Gameplay Insights tool shows it. Reads and Writes are component type ids.
    struct FSystemScheduleEntry
    {
        FName           Name;
        TVector<uint32> Writes;
        TVector<uint32> Reads;
        uint8           Stage      = 0;     // EUpdateStage
        uint8           Priority   = 255;
        uint8           Batch      = 0;     // Parallel batch index within the stage
        uint8           BatchSize  = 1;     // Systems running together in that batch
        bool            bExclusive = false;
        bool            bManaged   = false;
    };

    REFLECT()
    class RUNTIME_API CWorld : public CObject
    {
        GENERATED_BODY()

        friend class FWorldManager;
        friend struct SSystemContext;
        friend struct FWorldDebugInterface;
        friend struct SRenderComponent;

        // Defined only by tests, which drive systems by hand and need the context the world hands them.
        friend struct FWorldTestAccess;

    public:

        //~ Nested types.

        // One system as scheduled in one stage. Systems owns the instance and this only orders it.
        struct FStageSlot
        {
            CEntitySystem* System = nullptr;
            uint8          StagePriority = 255;
        };

        // Slots that may run together, as indices into SystemUpdateList.
        struct FSystemBatch
        {
            TVector<uint16> Members;
        };

        // One reflected engine system as the World Editor's Systems panel lists it.
        struct FSystemInfo
        {
            TSubclassOf<CEntitySystem> Class;
            FName                 Name;
            bool                  bEnabled = true;
            TVector<EUpdateStage> Stages;
            // The priority asked for in each of Stages, which a disabled system still reports.
            TVector<uint8>        Priorities;
        };

        //~ Construction and the CObject contract.

        CWorld();

        void Serialize(FArchive& Ar) override;

        // Both registries, so a world loaded but never initialized still shows what its entities hold.
        void VisitAdditionalObjectReferences(FObjectReferenceVisitor::FSlotFunc Func) override;

        void PreLoad() override;
        void PostLoad() override;

        bool IsAsset() const override { return true; }

        static CWorld* DuplicateWorld(CWorld* OwningWorld);

        //~ Lifecycle, from creation through the per-frame stages to teardown.

        // Must run before anything else is done with the world.
        void InitializeWorld(EWorldType InWorldType);

        // Destroys systems, components and entities.
        void TeardownWorld();

        // Runs the systems of the current update stage.
        void Update(const FUpdateContext& Context);

        // Steps physics on the game thread, between the DuringPhysics and PostPhysics stages.
        void TickPhysics();

        // Sends the contact events the step recorded, right after it.
        void DispatchPhysicsEvents();

        // Resolves the camera and post process and fills the scene's frame state, before RenderView reads it.
        void Extract();

        void SetActive(bool bNewActive);

        bool IsSuspended() const { return !bActive; }

        // Seconds between frames while throttled, where 0 runs every frame.
        void SetUpdateInterval(double Seconds);

        double GetUpdateInterval() const { return UpdateIntervalSeconds; }

        // Latched once a frame in FWorldManager::BeginFrame, so every update stage of a frame agrees.
        bool IsTickingThisFrame() const { return bActive && !bThrottledThisFrame; }

        // Frees the render scene once suspended longer than GraceSeconds, returning whether it did.
        bool ReclaimIdleRenderer(double NowSeconds, double GraceSeconds);

        //~ Identity and networking.

        FUNCTION()
        NODISCARD EWorldType GetWorldType() const { return WorldType; }

        FUNCTION()
        NODISCARD bool IsGameWorld() const { return WorldType == EWorldType::Game; }

        FUNCTION()
        NODISCARD bool IsSimulating() const { return WorldType == EWorldType::Simulation; }

        // Set once FWorldManager::CreateWorldContext has registered the world.
        NODISCARD FWorldContext* GetWorldContext() const { return OwningContext; }
        
        FUNCTION()
        EUpdateStage GetUpdateStage() const;

        // Standalone when the world has no context.
        FUNCTION()
        NODISCARD ENetMode GetNetMode() const;

        // A load-test client, which gameplay drives itself instead of reading a person's input.
        FUNCTION()
        NODISCARD bool IsBotWorld() const;

        // True on a listen or dedicated server.
        FUNCTION()
        NODISCARD bool IsNetServer() const;

        // Zero on clients and standalone worlds.
        FUNCTION()
        NODISCARD int32 GetConnectedClientCount() const;

        // False headless, on a dedicated server and in a bot world, so cosmetic work can skip all three.
        FUNCTION()
        NODISCARD bool HasPresentation() const;

        //~ Time, pausing and dilation.

        FUNCTION()
        double GetWorldDeltaTime() const { return DeltaTime; }

        // The frame's time before dilation, for things such as cinematics that keep pace with the clock.
        FUNCTION()
        double GetRealDeltaTime() const { return RealDeltaTime; }

        FUNCTION()
        double GetTimeSinceWorldCreation() const { return TimeSinceCreation; }

        // Pauses systems and physics, while UI and systems in EUpdateStage::Paused keep running.
        FUNCTION()
        void SetPaused(bool bNewPause) { bPaused = bNewPause; }

        FUNCTION()
        bool IsPaused() const { return bPaused; }

        // Whether a paused editor world still previews animation, which an asset preview may not want.
        FUNCTION()
        void SetAnimatesWhilePaused(bool bAnimate) { bAnimatesWhilePaused = bAnimate; }

        FUNCTION()
        bool AnimatesWhilePaused() const { return bAnimatesWhilePaused; }

        // True from the start of teardown, so an OnDetach can tell the world going away from its entity being removed.
        FUNCTION()
        bool IsTearingDown() const { return bTearingDown; }

        // Scales the delta time systems, scripts and physics see.
        FUNCTION()
        void SetTimeDilation(float Dilation);

        FUNCTION()
        float GetTimeDilation() const;

        //~ Entities.

        // A named entity with a transform. The name need not be unique.
        FUNCTION()
        ECS::FEntity ConstructEntity(FName Name, const FTransform& Transform = FTransform());

        // An entity with no components at all.
        NODISCARD ECS::FEntity CreateEntity() { return EntityRegistry.Create(); }

        FUNCTION()
        void DestroyEntity(ECS::FEntity Entity);

        void DuplicateEntity(ECS::FEntity& To, ECS::FEntity From, const TFunctionRef<bool(const ECS::FComponentTypeInfo&)>& Callback);

        // Deep copies Source and its children, returning the new root.
        FUNCTION()
        ECS::FEntity DuplicateEntity(ECS::FEntity Source);

        // Destroys Entity after Seconds, where 0 never does. A second call retimes it.
        FUNCTION()
        void SetEntityLifetime(ECS::FEntity Entity, float Seconds);

        FUNCTION(SuppressGCTransition)
        NODISCARD bool IsValidEntity(ECS::FEntity Entity) const { return EntityRegistry.IsValid(Entity); }

        FUNCTION()
        uint32 GetNumEntities() const;

        // Destroys every entity while the component storages keep their types.
        void ClearAllEntities() { EntityRegistry.Clear(); }

        // Shatters a destructible entity from Origin, where Strength 0 uses the component's own.
        FUNCTION()
        bool FractureEntity(ECS::FEntity Entity, FVector3 Origin, float Strength = 0.0f);

        //~ Names and tags.

        FUNCTION()
        ECS::FEntity GetEntityByName(const FName& Name);

        FUNCTION()
        FName GetEntityName(ECS::FEntity Entity);

        FUNCTION()
        ECS::FEntity GetEntityByTag(const FName& Tag);

        // None when the entity carries no tag.
        FUNCTION()
        FName GetEntityTag(ECS::FEntity Entity);

        // Appends every entity carrying Tag.
        FUNCTION()
        void GetEntitiesByTag(const FName& Tag, TVector<ECS::FEntity>& Out);

        FUNCTION()
        bool EntityHasTag(ECS::FEntity Entity, const FName& Tag);

        // An entity carries one tag, so this replaces whatever it had and None clears it.
        FUNCTION()
        void SetEntityTag(ECS::FEntity Entity, const FName& Tag);

        //~ Transforms. Getters read world space, and setters write the entity's own space under its parent.

        FUNCTION()
        FTransform GetEntityTransform(ECS::FEntity Entity);

        // Gives the entity a transform component when it has none.
        FUNCTION()
        void SetEntityTransform(ECS::FEntity Entity, const FTransform& NewTransform);

        FUNCTION()
        FVector3 GetEntityLocation(ECS::FEntity Entity);

        FUNCTION()
        FQuat GetEntityRotation(ECS::FEntity Entity);

        FUNCTION()
        FVector3 GetEntityScale(ECS::FEntity Entity);

        FUNCTION()
        void SetEntityLocation(ECS::FEntity Entity, FVector3 Location);

        FUNCTION()
        void SetEntityRotation(ECS::FEntity Entity, FQuat Rotation);

        FUNCTION()
        void SetEntityScale(ECS::FEntity Entity, FVector3 Scale);

        // One crossing and one lookup for placing an entity every frame.
        FUNCTION()
        void SetEntityLocationAndRotation(ECS::FEntity Entity, FVector3 Location, FQuat Rotation);

        FUNCTION()
        FVector3 TranslateEntity(ECS::FEntity Entity, FVector3 Translation);

        // One crossing for a pool of entities script moves every frame, pairing by index and ignoring extras.
        FUNCTION()
        void SetEntityTransforms(const ECS::FEntity* Entities, int32 EntityCount, const FTransform* Transforms, int32 TransformCount);

        //~ Hierarchy.

        // A null Parent detaches to the world root. The world transform is kept.
        FUNCTION()
        void SetParent(ECS::FEntity Child, ECS::FEntity Parent);

        FUNCTION()
        void DetachFromParent(ECS::FEntity Entity);

        FUNCTION()
        ECS::FEntity GetParent(ECS::FEntity Entity);

        FUNCTION()
        ECS::FEntity GetRootEntity(ECS::FEntity Entity);

        FUNCTION()
        ECS::FEntity GetFirstChild(ECS::FEntity Entity) const;

        FUNCTION()
        ECS::FEntity GetNextSibling(ECS::FEntity Entity) const;

        // Entity first, then each parent up to the root, in one crossing.
        FUNCTION()
        void GetAncestorChain(ECS::FEntity Entity, TVector<ECS::FEntity>& Out) const;

        // Entity and every descendant, in no particular order.
        FUNCTION()
        void GetSubtree(ECS::FEntity Entity, TVector<ECS::FEntity>& Out) const;

        NODISCARD const ECS::FHierarchy& GetHierarchy() const { return EntityRegistry.GetHierarchy(); }

        //~ Entity scripts.

        // Attaches and binds a script of ScriptClass, C++ or C#, returning it or null on failure.
        FUNCTION()
        CEntityScript* AddEntityScript(ECS::FEntity Entity, TSubclassOf<CEntityScript> ScriptClass);

        template<typename T>
        T* AddEntityScript(ECS::FEntity Entity);

        // The first script on Entity whose class derives ScriptClass, or null.
        FUNCTION()
        CEntityScript* FindEntityScript(ECS::FEntity Entity, TSubclassOf<CEntityScript> ScriptClass);

        FUNCTION()
        void FindEntityScripts(ECS::FEntity Entity, TSubclassOf<CEntityScript> ScriptClass, TVector<TStrongObjectPtr<CEntityScript>>& Out);

        // Every attached script in the world whose class derives ScriptClass, in no particular order.
        FUNCTION()
        void FindScriptsInWorld(TSubclassOf<CEntityScript> ScriptClass, TVector<TStrongObjectPtr<CEntityScript>>& Out);

        // Runs OnDetach and drops the script, false when it was not attached to Entity.
        FUNCTION()
        bool RemoveEntityScript(ECS::FEntity Entity, CEntityScript* Script);

        //~ Components.

        template<typename T, typename... TArgs>
        decltype(auto) EmplaceComponent(ECS::FEntity Entity, TArgs&&... Args);

        template<typename T, typename... TArgs>
        decltype(auto) EmplaceOrReplaceComponent(ECS::FEntity Entity, TArgs&&... Args);

        template<typename T, typename... TArgs>
        T& GetOrEmplaceComponent(ECS::FEntity Entity, TArgs&&... Args);

        template<typename T, typename TFunc>
        T& PatchComponent(ECS::FEntity Entity, TFunc&& Func);

        template<typename T> requires (!std::is_empty_v<T>)
        NODISCARD auto& GetComponent(this auto& Self, ECS::FEntity Entity);

        template<typename T>
        NODISCARD auto* TryGetComponent(this auto& Self, ECS::FEntity Entity);

        template<typename... T>
        NODISCARD bool HasComponent(ECS::FEntity Entity) const;

        template<typename... T>
        NODISCARD bool HasAnyComponent(ECS::FEntity Entity) const;

        template<typename... T>
        void RemoveComponent(ECS::FEntity Entity);

        template<typename T>
        void ClearComponents();

        // Every storage, for reflection-style passes over all of them.
        NODISCARD auto& ComponentStorages() const { return EntityRegistry.GetActiveStorages(); }

        template<typename T>
        NODISCARD auto& ComponentStorage();

        // A named storage, such as the one per entity tag.
        template<typename T>
        NODISCARD auto& NamedStorage(uint32 Id);

        //~ Views and component signals.

        template<typename... TGet>
        NODISCARD auto View();

        template<typename... TGet, typename... TExcluded>
        NODISCARD auto View(ECS::TExclude<TExcluded...> Exclude);

        // Keeps CObject's destroy hook visible beside the component signal of the same name.
        using CObject::OnDestroy;

        template<typename T>
        NODISCARD ECS::FComponentSignal& OnConstruct();

        template<typename T>
        NODISCARD ECS::FComponentSignal& OnDestroy();

        template<typename T>
        NODISCARD ECS::FComponentSignal& OnUpdate();

        NODISCARD ECS::FComponentSignal& OnEntityConstruct() { return EntityRegistry.OnEntityCreated(); }

        NODISCARD ECS::FComponentSignal& OnEntityDestroy() { return EntityRegistry.OnEntityDestroyed(); }

        //~ Singletons, one per type in the registry context.

        template<typename T, typename... TArgs>
        T& EmplaceSingleton(TArgs&&... Args);

        template<typename T>
        NODISCARD auto& GetSingleton(this auto& Self);

        template<typename T>
        T& GetOrEmplaceSingleton();

        template<typename T>
        NODISCARD auto* TryGetSingleton(this auto& Self);

        template<typename T>
        NODISCARD bool HasSingleton() const;

        template<typename T>
        void EraseSingleton();

        //~ Registry, deferred commands, events and messages.

        NODISCARD auto& GetEntityRegistry(this auto& Self);

        NODISCARD auto& GetCommandBus(this auto& Self);

        // Every event queue the world owns, sent to from anywhere and read by systems and scripts.
        NODISCARD auto& GetEvents(this auto& Self);

        NODISCARD auto& GetMessageBus(this auto& Self);

        // Commits sent events and applies recorded commands until both settle. Main thread, outside any batch.
        void FlushCommands();

        //~ Per-world managers.

        NODISCARD auto& GetTimerManager(this auto& Self);

        NODISCARD auto& GetTweenManager(this auto& Self);

        NODISCARD auto& GetCoroutineManager(this auto& Self);

        NODISCARD auto& GetInputModeLeases(this auto& Self);

        //~ Timers, on world time, by generational id so a stale one reports inactive.

        /** Runs OnFire after Rate seconds, repeating while bLoop. FirstDelay >= 0 overrides the first wait. */
        FUNCTION()
        uint32 SetTimer(float Rate, SScriptCallback OnFire, bool bLoop = false,
            float FirstDelay = -1.0f);

        /** As SetTimer, but cleared automatically when Owner is destroyed. */
        FUNCTION()
        uint32 SetTimerForEntity(ECS::FEntity Owner, float Rate, SScriptCallback OnFire,
            bool bLoop = false, float FirstDelay = -1.0f);

        FUNCTION()
        void ClearTimer(uint32 Timer);

        FUNCTION()
        bool IsTimerActive(uint32 Timer);

        /** Seconds until the next fire. */
        FUNCTION()
        float GetTimerRemaining(uint32 Timer);

        /** Seconds elapsed in the current interval. */
        FUNCTION()
        float GetTimerElapsed(uint32 Timer);

        FUNCTION()
        void SetTimerPaused(uint32 Timer, bool bTimerPaused);

        /** Stops every script-owned timer, so a looper does not tick on into the next generation. */
        FUNCTION()
        static void ClearAllManagedTimers();

        //~ Script waits on the coroutine scheduler. OnDone receives 1 once the wait elapses, or 0 when the world went away first.

        // Resumes after Ticks unpaused frames.
        FUNCTION()
        uint64 WaitTicks(int32 Ticks, SScriptCallback OnDone);

        // Resumes after Seconds measured on Clock.
        FUNCTION()
        uint64 WaitSeconds(double Seconds, ECoroutineClock Clock, SScriptCallback OnDone);

        // Drops a wait without calling it.
        FUNCTION()
        void CancelWait(uint64 Wait);

        // How many waits the world is holding, for leak checks.
        FUNCTION()
        int32 GetPendingWaitCount() const;

        //~ Event queues as script reaches them, with the typed reading and sending built over these in C#.

        // The queue type for a reflected event of that name, or a script event laid out as described. 0xFFFF when neither fits.
        FUNCTION(ScriptInternal)
        static uint32 ResolveEventType(FName Name, uint32 Size, uint32 Alignment);

        FUNCTION(ScriptInternal)
        void SendEvents(uint32 TypeId, void* Events, uint32 Count);

        // Fills Runs, laid out as ECS::FEventRuns, with what is unread past the uint64 at Cursor, and moves the cursor past it.
        FUNCTION(ScriptInternal)
        void ReadEvents(uint32 TypeId, void* Cursor, void* Runs);

        FUNCTION(ScriptInternal)
        uint32 NumUnreadEvents(uint32 TypeId, uint64 Cursor) const;

        //~ Gameplay messages, where typed sending and listening for C# is built over these.

        // The key C++ and C# senders share for a message type name.
        FUNCTION(ScriptInternal)
        static uint64 GetGameplayMessageTypeKey(FName TypeName);

        // The bus owns Listener from here, and releases it when the subscription goes.
        FUNCTION(ScriptInternal)
        uint64 SubscribeGameplayMessage(uint32 Channel, ECS::FEntity Owner, uint64 Type, bool bExactMatch, SScriptCallback Listener);

        FUNCTION(ScriptInternal)
        bool UnsubscribeGameplayMessage(uint64 Id);

        // Drops every listener Owner registered, which the bus also does on its own when the entity dies.
        FUNCTION()
        void UnsubscribeAllGameplayMessages(ECS::FEntity Owner);

        // A managed payload's handle belongs to the bus once sent, whether or not anything receives it.
        FUNCTION(ScriptInternal)
        void SendGameplayMessage(uint32 Route, ECS::FEntity Target, uint32 Channel, uint32 Kind, uint64 Type,
            void* Data, uint32 Size, void* Struct, bool bIncludeSelf);

        //~ Tweens, built step by step on a tween id from CreateTweenId.

        /** Starts an empty tween and returns its id. Owner ties its lifetime to that entity. */
        FUNCTION()
        uint32 CreateTweenId(ECS::FEntity Owner);

        FUNCTION()
        void TweenMoveTo(uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration);

        FUNCTION()
        void TweenRotateTo(uint32 Tween, ECS::FEntity Target, FQuat Destination, float Duration);

        FUNCTION()
        void TweenScaleTo(uint32 Tween, ECS::FEntity Target, FVector3 Destination, float Duration);

        // Relative to wherever Target is when the step begins. ScaleBy multiplies, so a factor of two doubles.
        FUNCTION()
        void TweenMoveBy(uint32 Tween, ECS::FEntity Target, FVector3 Offset, float Duration);

        FUNCTION()
        void TweenRotateBy(uint32 Tween, ECS::FEntity Target, FQuat Delta, float Duration);

        FUNCTION()
        void TweenScaleBy(uint32 Tween, ECS::FEntity Target, FVector3 Factor, float Duration);

        // A kick toward Punch that swings back and settles, rotation in degrees. Vibrato counts the swings.
        FUNCTION()
        void TweenPunch(uint32 Tween, ECS::FEntity Target, ETweenTransformChannel Channel, FVector3 Punch, float Duration, int32 Vibrato, float Elasticity);

        // A decaying random wobble of up to Strength on each axis, rotation in degrees.
        FUNCTION()
        void TweenShake(uint32 Tween, ECS::FEntity Target, ETweenTransformChannel Channel, FVector3 Strength, float Duration, int32 Vibrato);

        /** Interpolates a bare number, reporting each value to OnValue until the tween is destroyed. */
        FUNCTION()
        void TweenValueTo(uint32 Tween, float From, float To, float Duration, SScriptCallback OnValue);

        // The payload is the address of the value, read by the callback before it returns.
        FUNCTION()
        void TweenValueToVector2(uint32 Tween, FVector2 From, FVector2 To, float Duration, SScriptCallback OnValue);

        FUNCTION()
        void TweenValueToVector3(uint32 Tween, FVector3 From, FVector3 To, float Duration, SScriptCallback OnValue);

        FUNCTION()
        void TweenValueToVector4(uint32 Tween, FVector4 From, FVector4 To, float Duration, SScriptCallback OnValue);

        // Slerps, so a wide turn follows the arc.
        FUNCTION()
        void TweenValueToQuat(uint32 Tween, FQuat From, FQuat To, float Duration, SScriptCallback OnValue);

        // Drives a reflected property on Target's component, named by type and a dotted path, with no script call per frame.
        FUNCTION()
        void TweenPropertyToFloat(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, float To, float Duration);

        FUNCTION()
        void TweenPropertyToVector2(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FVector2 To, float Duration);

        FUNCTION()
        void TweenPropertyToVector3(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FVector3 To, float Duration);

        FUNCTION()
        void TweenPropertyToVector4(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FVector4 To, float Duration);

        FUNCTION()
        void TweenPropertyToQuat(uint32 Tween, ECS::FEntity Target, const FString& Component, const FString& Path, FQuat To, float Duration);

        /** Dead time, for spacing steps apart. */
        FUNCTION()
        void TweenInterval(uint32 Tween, float Duration);

        /** Runs OnCall when the sequence reaches this step. */
        FUNCTION()
        void TweenCall(uint32 Tween, SScriptCallback OnCall);

        /** Runs OnFinished after the last step, including after the final loop. */
        FUNCTION()
        void OnTweenFinished(uint32 Tween, SScriptCallback OnFinished);

        // Runs as each pass ends and another begins, with the passes completed so far as the payload.
        FUNCTION()
        void OnTweenLoop(uint32 Tween, SScriptCallback OnLoop);

        // Runs as each step ends, with that step's index as the payload.
        FUNCTION()
        void OnTweenStep(uint32 Tween, SScriptCallback OnStep);

        /** Transition, Ease and Delay all apply to the step that was added last. */
        FUNCTION()
        void TweenTrans(uint32 Tween, EEaseTransition Transition);

        FUNCTION()
        void TweenEase(uint32 Tween, EEaseType Ease);

        // Shapes the last step with the curve over its keyed time range, replacing Trans and Ease.
        FUNCTION()
        void TweenEaseCurve(uint32 Tween, CCurveAsset* Curve);

        FUNCTION()
        void TweenDelay(uint32 Tween, float Seconds);

        /** Runs the last step alongside the one before it rather than after it. */
        FUNCTION()
        void TweenParallel(uint32 Tween);

        /** Zero or less loops forever. */
        FUNCTION()
        void SetTweenLoops(uint32 Tween, int32 Count);

        FUNCTION()
        void SetTweenLoopMode(uint32 Tween, ETweenLoopMode Mode);

        FUNCTION()
        void SetTweenSpeedScale(uint32 Tween, float Scale);

        // Runs on real time, through world pause and time dilation.
        FUNCTION()
        void SetTweenIgnoreTimeScale(uint32 Tween, bool bIgnore);

        FUNCTION()
        void SetTweenPaused(uint32 Tween, bool bTweenPaused);

        /** Stops where it is; whatever it was driving keeps its current value. */
        FUNCTION()
        void KillTween(uint32 Tween);

        // Jumps to the end of the current pass, running every step and OnFinished on the way.
        FUNCTION()
        void CompleteTween(uint32 Tween);

        FUNCTION()
        bool IsTweenRunning(uint32 Tween);

        // Seconds into the current pass.
        FUNCTION()
        float GetTweenElapsed(uint32 Tween);

        // The length of one pass.
        FUNCTION()
        float GetTweenDuration(uint32 Tween);

        FUNCTION()
        float GetTweenProgress(uint32 Tween);

        FUNCTION()
        int32 GetTweenCompletedLoops(uint32 Tween);

        //~ Input, read from the world's own viewport, and neutral while the world is not receiving it.

        /** False when the world has no viewport, is not the active one, or the editor holds input focus. */
        FUNCTION()
        bool IsReceivingInput();

        //~ Authored actions, the rebindable surface.

        /** This frame's evaluated state, zeroed when the name is not an authored action. */
        FUNCTION()
        SInputActionState GetActionState(const FName& Action);

        FUNCTION()
        bool IsActionDown(const FName& Action);

        FUNCTION()
        bool WasActionPressed(const FName& Action);

        FUNCTION()
        bool WasActionReleased(const FName& Action);

        FUNCTION()
        bool IsActionHeld(const FName& Action);

        FUNCTION()
        bool WasActionTapped(const FName& Action);

        FUNCTION()
        bool WasActionDoubleTapped(const FName& Action);

        /** True on the press and again each time a repeat comes due while held. */
        FUNCTION()
        bool IsActionRepeated(const FName& Action);

        FUNCTION()
        bool IsActionBuffered(const FName& Action);

        /** Takes the buffered press so it acts once, returning whether there was one. */
        FUNCTION()
        bool ConsumeBufferedPress(const FName& Action);

        FUNCTION()
        float GetActionAxis(const FName& Action);

        FUNCTION()
        FVector2 GetActionAxis2D(const FName& Action);

        FUNCTION()
        float GetActionHeldTime(const FName& Action);

        /** Plus one while Positive is down and minus one while Negative is, so holding both cancels. */
        FUNCTION()
        float GetAxisPair(const FName& Positive, const FName& Negative);

        //~ Mapping layers, per world. A blocking layer stops any action it does not list.

        FUNCTION()
        void PushLayer(const FName& Layer);

        /** False when the layer was not on the stack. */
        FUNCTION()
        bool PopLayer(const FName& Layer);

        FUNCTION()
        bool HasLayer(const FName& Layer);

        FUNCTION()
        void ClearLayers();

        //~ Raw device state, shared by every entity in the world and not rebindable.

        FUNCTION()
        bool IsKeyDown(EKey Key);

        FUNCTION()
        bool WasKeyPressed(EKey Key);

        FUNCTION()
        bool WasKeyReleased(EKey Key);

        FUNCTION()
        bool IsMouseButtonDown(EMouseKey Button);

        FUNCTION()
        bool WasMouseButtonPressed(EMouseKey Button);

        FUNCTION()
        bool WasMouseButtonReleased(EMouseKey Button);

        FUNCTION()
        FVector2 GetMousePosition();

        FUNCTION()
        FVector2 GetMouseDelta();

        FUNCTION()
        float GetMouseWheel();

        /** False when no pad is connected, or the world is not receiving input. */
        FUNCTION()
        bool IsGamepadConnected();

        FUNCTION()
        bool IsGamepadButtonDown(EGamepadButton Button);

        FUNCTION()
        bool WasGamepadButtonPressed(EGamepadButton Button);

        FUNCTION()
        bool WasGamepadButtonReleased(EGamepadButton Button);

        /** Dead-zoned, with stick Y positive up and triggers from 0 to 1. */
        FUNCTION()
        float GetGamepadAxis(EGamepadAxis Axis);

        FUNCTION()
        FVector2 GetGamepadLeftStick();

        FUNCTION()
        FVector2 GetGamepadRightStick();

        //~ Prompts, which follow whichever device the player touched last.

        FUNCTION()
        EInputDevice GetLastUsedDevice();

        /** Such as "E" or "Cross", and empty when the action has no binding on the last used device. */
        FUNCTION()
        FString GetActionPrompt(const FName& Action);

        //~ Cursor and routing, which act on the world's own viewport rather than the receiving one.

        FUNCTION()
        void SetInputMode(EInputMode Mode);

        FUNCTION()
        EInputMode GetInputMode();

        FUNCTION()
        void SetMouseMode(EMouseMode Mode);

        FUNCTION()
        EMouseMode GetMouseMode();

        // Holds the modes until the returned lease is released, combined with every other holder's request.
        FUNCTION()
        uint64 AcquireInputMode(EInputMode InputMode, EMouseMode MouseMode);

        FUNCTION()
        bool ReleaseInputMode(uint64 Lease);

        //~ Animation, on an entity's animation graph and montages.

        //~ The simple single clip backend, SSimpleAnimationComponent.

        /** Adds the component if the entity has none, which is why this is the one call that cannot no-op. */
        FUNCTION()
        void PlayAnimation(ECS::FEntity Entity, CAnimation* Clip, bool bLoop = false,
            float Speed = 1.0f);

        FUNCTION()
        void StopAnimation(ECS::FEntity Entity);

        FUNCTION()
        void PauseAnimation(ECS::FEntity Entity);

        FUNCTION()
        void ResumeAnimation(ECS::FEntity Entity);

        FUNCTION()
        bool IsAnimationPlaying(ECS::FEntity Entity);

        FUNCTION()
        bool IsAnimationFinished(ECS::FEntity Entity);

        FUNCTION()
        void SetAnimationSpeed(ECS::FEntity Entity, float Speed);

        FUNCTION()
        void SetAnimationTime(ECS::FEntity Entity, float Time);

        FUNCTION()
        float GetAnimationTime(ECS::FEntity Entity);

        //~ Graph parameters, which live as fields on the graph's own parameter struct.

        FUNCTION()
        void SetAnimFloat(ECS::FEntity Entity, const FName& Name, float Value);

        FUNCTION()
        float GetAnimFloat(ECS::FEntity Entity, const FName& Name, float Default = 0.0f);

        FUNCTION()
        void SetAnimBool(ECS::FEntity Entity, const FName& Name, bool bValue);

        FUNCTION()
        bool GetAnimBool(ECS::FEntity Entity, const FName& Name, bool bDefault = false);

        FUNCTION()
        bool HasAnimParameter(ECS::FEntity Entity, const FName& Name);

        //~ Montages. The graph must contain a matching slot node for anything to show.

        /** Zero when it did not start. An empty section starts at the montage's first one. */
        FUNCTION()
        uint32 PlayMontage(ECS::FEntity Entity, CAnimationMontage* Montage,
            float PlayRate = 1.0f, const FName& Section = FName());

        /** A null montage stops every one playing on the entity. */
        FUNCTION()
        void StopMontage(ECS::FEntity Entity, CAnimationMontage* Montage,
            float BlendOutTime = -1.0f);

        FUNCTION()
        bool JumpToMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage,
            const FName& Section);

        FUNCTION()
        bool SetNextMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage,
            const FName& Section);

        /** A null montage asks whether any montage is playing at all. */
        FUNCTION()
        bool IsMontagePlaying(ECS::FEntity Entity, CAnimationMontage* Montage);

        FUNCTION()
        float GetMontagePosition(ECS::FEntity Entity, CAnimationMontage* Montage);

        FUNCTION()
        float GetMontageWeight(ECS::FEntity Entity, CAnimationMontage* Montage);

        FUNCTION()
        void SetMontagePlayRate(ECS::FEntity Entity, CAnimationMontage* Montage,
            float PlayRate);

        FUNCTION()
        FName GetMontageSection(ECS::FEntity Entity, CAnimationMontage* Montage);

        //~ Navigation. With no baked navmesh every query reports not found, and an empty Agent means the infantry default.

        /** True once a navmesh has been baked and finished hydrating, so queries can succeed. */
        FUNCTION()
        bool IsNavigationReady(FName Agent = FName());

        /** Writes the route's corners into the caller's buffer and returns how many it wrote. */
        FUNCTION()
        int32 FindPath(FVector3 Start, FVector3 End, FVector3* Corners, int32 MaxCorners, FName Agent = FName());

        /** Whether the last route between these points stops short of the goal, for any reason. */
        FUNCTION()
        bool IsPathPartial(FVector3 Start, FVector3 End, int32 MaxCorners, FName Agent = FName());

        // Why a route between these points ended as it did, so a failure can say which part failed.
        FUNCTION()
        ENavPathResult GetPathResult(FVector3 Start, FVector3 End, int32 MaxCorners, FName Agent = FName());

        /** Snaps onto the nearest walkable surface within the half-extents search box. */
        FUNCTION()
        SNavPoint ProjectPointToNavigation(FVector3 Point, FVector3 Extents, FName Agent = FName());

        /** Walks the surface toward End and reports where a wall stopped it. */
        FUNCTION()
        SNavRaycastResult NavRaycast(FVector3 Start, FVector3 End, FName Agent = FName());

        /** Whether the straight line stays on walkable surface the whole way, far cheaper than a path. */
        FUNCTION()
        bool IsWalkableLine(FVector3 From, FVector3 To, FName Agent = FName());

        FUNCTION()
        SNavPoint FindRandomReachablePoint(FVector3 Origin, float Radius, FName Agent = FName());

        /** Whether a complete route exists, so a partial one reports false. */
        FUNCTION()
        bool IsReachable(FVector3 From, FVector3 To, FName Agent = FName());

        /** Total route length in world units, or a negative value when unreachable. */
        FUNCTION()
        float GetPathLength(FVector3 From, FVector3 To, FName Agent = FName());

        /** Flags every navmesh volume for an async rebuild and returns how many it flagged. */
        FUNCTION()
        int32 RequestNavigationRebuild();

        FUNCTION()
        void DrawNavPath(FVector3 From, FVector3 To, FVector4 Color, float Duration = 0.0f, FName Agent = FName());

        //~ Sequences and their events.

        // Spawns a player entity for Sequence and starts it, returning the player so it can be paused, sought or stopped.
        FUNCTION()
        ECS::FEntity PlaySequence(CSequence* Sequence, bool bLoop = false, float PlayRate = 1.0f);

        // Stops the player and destroys its entity, putting back whatever the sequence displaced.
        FUNCTION()
        void StopSequence(ECS::FEntity Player);

        FUNCTION()
        bool IsSequencePlaying(ECS::FEntity Player);

        // Every event any sequence in the world fired since the last call, oldest first.
        void ConsumeSequenceEvents(TVector<SSequenceFiredEvent>& OutEvents);

        // Steps to the next event any sequence fired, false once there are none, then the getters below describe it.
        FUNCTION()
        bool NextSequenceEvent();

        FUNCTION()
        FString GetSequenceEventName();

        FUNCTION()
        FString GetSequenceEventPayload();

        FUNCTION()
        ECS::FEntity GetSequenceEventTarget();

        FUNCTION()
        float GetSequenceEventTime();

        // Whether any sequence is playing, so gameplay can hide its HUD and hold its input during a cinematic.
        FUNCTION()
        bool IsAnySequencePlaying();

        // A fade toward Color and black bars at LetterboxAspect over whichever camera is live, zero clearing each.
        FUNCTION()
        void SetCinematicOverlay(float Fade, FVector3 Color, float LetterboxAspect);

        //~ Skeletal meshes, bones and sockets.

        /** Parents Child under Parent and keeps it glued to the named socket each frame. */
        FUNCTION()
        void AttachEntityToSocket(ECS::FEntity Child, ECS::FEntity Parent,
            const FName& SocketOrBone);

        /** Stops following the socket and detaches to the world root, preserving the world transform. */
        FUNCTION()
        void DetachEntityFromSocket(ECS::FEntity Entity);

        FUNCTION()
        bool HasSocket(ECS::FEntity Entity, const FName& SocketOrBone);

        /** World-space socket location, zero when it does not resolve. */
        FUNCTION()
        FVector3 GetSocketLocation(ECS::FEntity Entity, const FName& SocketOrBone);

        /** World-space socket rotation, identity when it does not resolve. */
        FUNCTION()
        FQuat GetSocketRotation(ECS::FEntity Entity, const FName& SocketOrBone);

        /** Bone name for a skeleton bone index such as a hit result's, NAME_None when out of range. */
        FUNCTION()
        FName GetBoneName(ECS::FEntity Entity, int32 BoneIndex);

        FUNCTION()
        int32 GetBoneIndex(ECS::FEntity Entity, const FName& BoneName);

        /** Bone origin nearest WorldLocation, which approximates the hit bone on a single-body mesh. */
        FUNCTION()
        FName FindClosestBone(ECS::FEntity Entity, FVector3 WorldLocation);

        /** Drives Follower's skeletal mesh from Leader's pose instead of its own animation. A null Leader follows the nearest parent with a skeletal mesh. */
        FUNCTION()
        void SetLeaderPose(ECS::FEntity Follower, ECS::FEntity Leader);

        /** Stops Follower copying a leader, so its own animation drives it again. */
        FUNCTION()
        void ClearLeaderPose(ECS::FEntity Follower);

        /** The entity whose pose Follower ends up copying, null when it follows nothing. */
        FUNCTION()
        ECS::FEntity GetLeaderPose(ECS::FEntity Follower);

        //~ Networking.

        // This peer's own connection, which is 0 on a host and in a standalone world.
        FUNCTION()
        uint32 GetLocalNetConnection();

        // The remote connections that finished joining, which only a host can see.
        FUNCTION()
        void GetNetConnections(TVector<uint32>& OutConnections);

        // Needs an SNetworkComponent on the entity.
        FUNCTION()
        bool IsEntityNetworked(ECS::FEntity Entity);

        FUNCTION()
        uint32 GetNetOwner(ECS::FEntity Entity);

        // Host only, a client asking is refused.
        FUNCTION()
        bool SetNetOwner(ECS::FEntity Entity, uint32 ConnectionId);

        // Resends the entity's replicated component fields.
        FUNCTION()
        void MarkEntityNetDirty(ECS::FEntity Entity);

        FUNCTION()
        uint32 EntityToNetId(ECS::FEntity Entity);

        FUNCTION()
        ECS::FEntity NetIdToEntity(uint32 NetId);

        // A host is always joined, and a client is once its handshake finished.
        FUNCTION()
        bool IsNetJoined();

        // The entity a connection owns, preferring one with a character controller.
        FUNCTION()
        ECS::FEntity GetOwnedPawn(uint32 ConnectionId);

        //~ AI perception.

        /** The targets the perceiver currently senses, empty when it has no perception component. */
        FUNCTION()
        void GetPerceivedTargets(ECS::FEntity Perceiver, TVector<ECS::FEntity>& Out);

        /** Whether any of the named sense channels currently detects the target. */
        FUNCTION()
        bool CanSense(ECS::FEntity Perceiver, ECS::FEntity Target, EAISenseChannel Senses);

        /** Held through the forget window, and not found once the target drops out of memory. */
        FUNCTION()
        SNavPoint GetLastKnownLocation(ECS::FEntity Perceiver, ECS::FEntity Target);

        /** Nearest perceived target to the perceiver's eye, or the null entity. */
        FUNCTION()
        ECS::FEntity GetClosestPerceivedTarget(ECS::FEntity Perceiver);

        /** One shot line of sight test, applying the eye and aim offsets when those components exist. */
        FUNCTION()
        bool HasPerceptionLineOfSight(ECS::FEntity From, ECS::FEntity To);

        /** Heard by every perceiver in range that detects the instigator's affiliation. */
        FUNCTION()
        void ReportPerceptionNoise(FVector3 Location, float Loudness, ECS::FEntity Instigator);

        /** The victim immediately perceives its attacker, whatever their affiliation. */
        FUNCTION()
        void ReportPerceptionDamage(ECS::FEntity Victim, ECS::FEntity Instigator,
            FVector3 HitLocation, float Amount);

        /** Makes the entity sensible. Richer affiliation sets are authored in the editor. */
        FUNCTION()
        void RegisterPerceptionSource(ECS::FEntity Entity, const FName& AffiliationTag,
            EAISenseChannel Senses);

        /** Only sources carrying a matching tag are sensed, and an empty filter senses everyone. */
        FUNCTION()
        void AddDetectableTag(ECS::FEntity Perceiver, const FName& Tag);

        //~ UI documents.

        /** UI pixel size of the world's screen context, which in the editor is the panel, not the target. */
        FUNCTION()
        FVector2 GetUILayoutSize();

        /** True while the cursor is on an interface element, so a click to move can ignore that click. */
        FUNCTION()
        bool IsCursorOverUI();

        //~ Documents

        /** Loads a virtual path into the world's screen context. The document starts hidden. */
        FUNCTION()
        SUIDocument LoadUIDocument(const FString& Path);

        /** SourceUrl resolves the document's relative includes such as stylesheets and images. */
        FUNCTION()
        SUIDocument LoadUIDocumentFromMemory(const FString& Rml, const FString& SourceUrl);

        FUNCTION()
        void UnloadUIDocument(SUIDocument Document);

        //~ Event listeners

        FUNCTION()
        SUIEventListener AddUIEventListener(SUIElement Element, const FString& EventType);

        FUNCTION()
        void RemoveUIEventListener(SUIEventListener Listener);

        //~ Data models

        // OnSet gets an FUIDataSet and OnEvent an FUIDataEvent by address, and the model owns both from here.
        FUNCTION(ScriptInternal)
        SUIDataModel CreateUIDataModel(const FString& Name, SScriptCallback OnSet, SScriptCallback OnEvent);

        //~ Foliage.

        // The world's single foliage entity, created on first use.
        FUNCTION()
        ECS::FEntity GetOrCreateFoliage();

        // Registers a mesh as a foliage type and returns the index instances refer to it by.
        FUNCTION()
        int32 AddFoliageType(CStaticMesh* Mesh, bool bCastShadow = true, float CullDistance = 0.0f);

        FUNCTION()
        void AddFoliageInstances(const SFoliageInstance* Instances, int32 Count);

        // Removes instances of a type within Radius on the ground plane, or of every type when TypeFilter is negative.
        FUNCTION()
        int32 RemoveFoliageInRadius(FVector3 Center, float Radius, int32 TypeFilter = -1);

        FUNCTION()
        int32 GetFoliageInstanceCount();

        // Hides or shows one instance without rebaking the field, for a felled tree or an opened door.
        FUNCTION()
        void SetFoliageInstanceHidden(int32 Index, bool bHidden);

        //~ Gameplay tags on entities.

        /** The registered tag ids the entity carries, empty when it has no tag component. */
        FUNCTION()
        void GetGameplayTags(ECS::FEntity Entity, TVector<uint32>& Out);

        //~ Stored on the tag component AddGameplayTag creates.

        FUNCTION()
        void AddGameplayTag(ECS::FEntity Entity, uint32 TagId);

        FUNCTION()
        void RemoveGameplayTag(ECS::FEntity Entity, uint32 TagId);

        /** Hierarchical, so an entity tagged with a leaf answers true for a query on its parent. */
        FUNCTION()
        bool HasGameplayTag(ECS::FEntity Entity, uint32 TagId);

        FUNCTION()
        bool HasGameplayTagExact(ECS::FEntity Entity, uint32 TagId);

        FUNCTION()
        void ClearGameplayTags(ECS::FEntity Entity);

        //~ Prefab spawning.

        /** Instantiates at the origin, unparented. */
        FUNCTION()
        ECS::FEntity SpawnPrefab(const TSoftObjectPtr<CPrefab>& Prefab);

        /** Positions the spawned root at SpawnTransform, under Parent when it is not the null entity. */
        FUNCTION()
        ECS::FEntity SpawnPrefabAt(const TSoftObjectPtr<CPrefab>& Prefab,
            const FTransform& SpawnTransform, ECS::FEntity Parent);

        // A prefab the caller already holds, at the origin and unparented, with no path lookup or load.
        FUNCTION()
        ECS::FEntity SpawnLoadedPrefab(CPrefab* Prefab);

        // A prefab the caller already holds, at SpawnTransform and under Parent when it is not the null entity.
        FUNCTION()
        ECS::FEntity SpawnLoadedPrefabAt(CPrefab* Prefab, const FTransform& SpawnTransform, ECS::FEntity Parent);

        /** Loads the prefab off the game thread, then spawns and reports it once, back on the game thread. */
        FUNCTION()
        void SpawnPrefabAsync(const TSoftObjectPtr<CPrefab>& Prefab, SScriptCallback OnSpawned);

        //~ Terrain.

        FUNCTION()
        bool IsOnTerrain(float X, float Z);

        // The terrain surface height under (X, Z), or Fallback when no terrain covers the point.
        FUNCTION()
        float GetTerrainHeight(float X, float Z, float Fallback = 0.0f);

        // The terrain surface normal under (X, Z), or world up when no terrain covers the point.
        FUNCTION()
        FVector3 GetTerrainNormal(float X, float Z);

        //~ Particle systems.

        // Bursts at SpawnTransform and despawns after Lifetime seconds, where zero leaves it to the caller and negative means the one-shot length.
        FUNCTION()
        ECS::FEntity SpawnParticleSystem(CParticleSystem* ParticleSystem,
            const FTransform& SpawnTransform, float Lifetime);

        /** The same, parented to Parent on a named socket or bone, where none means the entity origin. */
        FUNCTION()
        ECS::FEntity SpawnParticleSystemAttached(CParticleSystem* ParticleSystem,
            ECS::FEntity Parent, const FName& Socket, FVector3 Offset, float Lifetime);

        // Where the particles of emitters flagged bReportCollisions hit, a few frames ago. A negative index takes every emitter.
        FUNCTION()
        void GetParticleCollisions(ECS::FEntity Entity, int32 EmitterIndex, TVector<SParticleCollision>& OutCollisions);

        //~ Save games.

        // Captures World into SaveGame and writes it at the end of the frame, safe from any system or thread.
        FUNCTION()
        void RequestSaveGame(CSaveGame* SaveGame, const FString& SlotName, int32 UserIndex);

        // Loads the slot and restores World at the end of the frame. SSaveGameCompletedEvent and OnWorldRestored report back.
        FUNCTION()
        void RequestLoadGame(const FString& SlotName, int32 UserIndex);

        //~ Levels.

        // The swap runs at the next frame start, so calling this mid tick is safe.
        FUNCTION()
        void OpenLevel(const FString& Url);

        //~ Meshes built at runtime.

        // Script geometry as a real static mesh, so it can be instanced, used as foliage or fractured. The world keeps it alive.
        FUNCTION()
        CStaticMesh* CreateStaticMesh(const float* Positions, int32 PositionFloats,
            const float* Normals, int32 NormalFloats, const float* UVs, int32 UVFloats,
            const float* Colors, int32 ColorFloats, const uint32* Indices, int32 IndexCount,
            CMaterialInterface* Material, int32 MaxLODs = 1);

        //~ Projectiles.

        /** A Lifetime above zero despawns the projectile through the engine lifetime system. */
        FUNCTION()
        ECS::FEntity SpawnProjectile(FVector3 Position, FVector3 Velocity, float Damage,
            float Lifetime, ECS::FEntity Instigator);

        //~ Destruction.

        // Shatters a mesh at a transform with no entity to set up, for something that turns to rubble the moment it falls.
        FUNCTION()
        bool ShatterMesh(CStaticMesh* Mesh, FTransform Transform, FVector3 Origin, float Strength = 6.0f,
                                int32 Pieces = 12, float Lifetime = 8.0f);

        //~ Decals.

        // Size.z is the depth the box reaches either side of the surface, and a positive Lifetime fades then destroys it.
        FUNCTION()
        ECS::FEntity SpawnDecal(CMaterialInterface* Material, FVector3 Location, FVector3 Normal, FVector3 Size,
                                       float RotationDegrees = 0.0f, float Lifetime = 0.0f, float FadeOutDuration = 1.0f,
                                       int32 AtlasCell = 0, int32 AtlasColumns = 1, int32 AtlasRows = 1);

        //~ Systems.

        void RegisterSystems();

        // Every reflected engine system by name, with its pending enabled state so a UI checkbox updates at once.
        void GetAllSystems(TVector<FSystemInfo>& Out) const;

        // Reads the pending state, so it answers a SetSystemEnabled made this frame.
        FUNCTION()
        bool IsSystemEnabled(TSubclassOf<CEntitySystem> System) const;

        // Persists at once and rebuilds the live system list at the start of the next frame, so it is safe mid-frame.
        FUNCTION()
        void SetSystemEnabled(TSubclassOf<CEntitySystem> System, bool bEnabled);

        // The last frame's per-stage batches with each system's declared access, for Gameplay Insights. Main thread.
        void GetSystemSchedule(TVector<FSystemScheduleEntry>& Out) const;

        //~ Subsystems, one per class per world, so a C++ and a C# one are the same thing to the world.

        // Matches a derived class too, and is null when the world has none.
        FUNCTION()
        NODISCARD CWorldSubsystem* GetSubsystem(TSubclassOf<CWorldSubsystem> Class) const { return WorldSubsystems::Find(Subsystems, Class.Get()); }

        template<typename T>
        NODISCARD T* GetSubsystem() const;

        NODISCARD const auto& GetSubsystems() const { return Subsystems; }

        //~ Physics.

        Physics::IPhysicsScene* GetPhysicsScene() const { return PhysicsScene.get(); }

        // Editor worlds skip the physics scene at init since it reserves hundreds of MB, so a tool that simulates asks here.
        Physics::IPhysicsScene* EnsurePhysicsScene();

        // Fixed script updates ride the solver's step loop, since a force applied elsewhere is zeroed.
        void InstallPhysicsScriptHook();

        //~ Ray and shape queries.

        // A miss keeps Start and End, while bHit marks whether the remaining hit fields are valid.
        FUNCTION()
        SRayResult Raycast(FVector3 Start, FVector3 End,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        /** Every body the segment crosses, sorted near to far, one entry per body. */
        FUNCTION()
        void RaycastAll(FVector3 Start, FVector3 End, ECS::FEntity IgnoreEntity,
            ECollisionProfiles LayerMask, TVector<SRayResult>& Out);

        // Raycast from Start along Direction for Distance meters.
        FUNCTION()
        SRayResult RaycastDirection(FVector3 Start, FVector3 Direction, float Distance,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // True when nothing blocks the segment, stopping at the first blocker. Both entities are skipped, the looker and what it looks at.
        FUNCTION()
        bool HasLineOfSight(FVector3 From, FVector3 To, ECS::FEntity IgnoreEntity,
            ECS::FEntity TargetEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // The segment against one entity's body only, such as testing whether a click landed on it.
        FUNCTION()
        SRayResult RaycastEntity(ECS::FEntity Entity, FVector3 Start, FVector3 End);

        //~ Sweeps move a shape along the segment, and the hit Location is where it first touches something.

        /** Every body a sphere of Radius touches along the segment, near to far. */
        FUNCTION()
        void SphereCast(FVector3 Start, FVector3 End, float Radius,
            ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        SRayResult SphereCastClosest(FVector3 Start, FVector3 End, float Radius,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        SRayResult BoxCast(FVector3 Start, FVector3 End, FVector3 HalfExtents, FQuat Rotation,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        void BoxCastAll(FVector3 Start, FVector3 End, FVector3 HalfExtents, FQuat Rotation,
            ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // The capsule runs along Rotation's up axis, the shape of a character moving through the world.
        FUNCTION()
        SRayResult CapsuleCast(FVector3 Start, FVector3 End, float Radius, float HalfHeight, FQuat Rotation,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        void CapsuleCastAll(FVector3 Start, FVector3 End, float Radius, float HalfHeight, FQuat Rotation,
            ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // Any shape, for a caller that picks the shape at runtime.
        FUNCTION()
        SRayResult ShapeCast(SQueryShape Shape, FVector3 Start, FVector3 End,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        void ShapeCastAll(SQueryShape Shape, FVector3 Start, FVector3 End,
            ECS::FEntity IgnoreEntity, TVector<SRayResult>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        //~ Overlaps list distinct entities whose bodies intersect the shape. IgnoreEntity excludes the querier.

        FUNCTION()
        void OverlapSphere(FVector3 Center, float Radius, ECS::FEntity IgnoreEntity,
            TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        void OverlapBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
            ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        void OverlapCapsule(FVector3 Center, float Radius, float HalfHeight, FQuat Rotation,
            ECS::FEntity IgnoreEntity, TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        void OverlapShape(SQueryShape Shape, FVector3 Center, ECS::FEntity IgnoreEntity,
            TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        /** Entities whose bodies contain the point, which is volume containment rather than a shape sweep. */
        FUNCTION()
        void OverlapPoint(FVector3 Point, ECS::FEntity IgnoreEntity,
            TVector<ECS::FEntity>& Out, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // True when no body overlaps the sphere, the check for whether something can be placed there.
        FUNCTION()
        bool IsSpaceFree(FVector3 Center, float Radius, ECS::FEntity IgnoreEntity,
            ECollisionProfiles LayerMask = ECollisionProfiles::All);

        FUNCTION()
        bool IsBoxSpaceFree(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // Whether a character capsule fits at Center, such as before a spawn or a teleport.
        FUNCTION()
        bool IsCapsuleSpaceFree(FVector3 Center, float Radius, float HalfHeight, FQuat Rotation,
            ECS::FEntity IgnoreEntity, ECollisionProfiles LayerMask = ECollisionProfiles::All);

        //~ Proximity, measured to the surface of an entity's colliders rather than to its origin.

        // The entity in Radius whose surface is closest to Center, or the null entity when there is none.
        FUNCTION()
        ECS::FEntity FindNearestEntity(FVector3 Center, float Radius, ECS::FEntity IgnoreEntity,
            ECollisionProfiles LayerMask = ECollisionProfiles::All);

        // Point itself when it is inside the body, and Point unchanged when the entity has no body.
        FUNCTION()
        FVector3 GetClosestPoint(ECS::FEntity Entity, FVector3 Point);

        // Zero inside the body, and negative when the entity has no body.
        FUNCTION()
        float GetDistanceToEntity(ECS::FEntity Entity, FVector3 Point);

        // The world bounds of the entity's colliders, an invalid box when it has no body.
        FUNCTION()
        SAABB GetBodyBounds(ECS::FEntity Entity);

        //~ Shape and settings. A direct field write on a physics component takes effect once the entity is rebuilt.

        // Re-applies the entity's body, collider and character settings after their fields were written directly, keeping its motion.
        FUNCTION()
        void RebuildPhysics(ECS::FEntity Entity);

        // Resizes the entity's character capsule, or its capsule collider when it is not a character.
        FUNCTION()
        void SetCapsuleSize(ECS::FEntity Entity, float Radius, float HalfHeight);

        FUNCTION()
        void SetBoxHalfExtent(ECS::FEntity Entity, FVector3 HalfExtent);

        FUNCTION()
        void SetSphereRadius(ECS::FEntity Entity, float Radius);

        // Moves the entity's body or character onto another profile, which changes what it collides with.
        FUNCTION()
        void SetCollisionProfile(ECS::FEntity Entity, SCollisionProfile Profile);

        // The height of the first surface straight down through Point, searching SearchDistance above and below it.
        FUNCTION()
        float GetGroundHeight(FVector3 Point, float SearchDistance, ECS::FEntity IgnoreEntity, float Fallback,
            ECollisionProfiles LayerMask = ECollisionProfiles::All);

        //~ Forces and impulses. An entity without a body is a no-op.

        FUNCTION()
        void AddForce(ECS::FEntity Entity, FVector3 Force);

        FUNCTION()
        void AddImpulse(ECS::FEntity Entity, FVector3 Impulse);

        FUNCTION()
        void AddTorque(ECS::FEntity Entity, FVector3 Torque);

        FUNCTION()
        void AddAngularImpulse(ECS::FEntity Entity, FVector3 AngularImpulse);

        FUNCTION()
        void AddForceAtPosition(ECS::FEntity Entity, FVector3 Force, FVector3 Position);

        FUNCTION()
        void AddImpulseAtPosition(ECS::FEntity Entity, FVector3 Impulse, FVector3 Position);

        //~ Body state. A getter on an entity without a body returns zero.

        FUNCTION()
        void SetLinearVelocity(ECS::FEntity Entity, FVector3 Velocity);

        FUNCTION()
        FVector3 GetLinearVelocity(ECS::FEntity Entity);

        FUNCTION()
        void SetAngularVelocity(ECS::FEntity Entity, FVector3 Velocity);

        FUNCTION()
        FVector3 GetAngularVelocity(ECS::FEntity Entity);

        FUNCTION()
        FVector3 GetVelocityAtPoint(ECS::FEntity Entity, FVector3 Point);

        FUNCTION()
        FVector3 GetBodyPosition(ECS::FEntity Entity);

        FUNCTION()
        FQuat GetBodyRotation(ECS::FEntity Entity);

        FUNCTION()
        FVector3 GetCenterOfMass(ECS::FEntity Entity);

        // Zero for a body that cannot move, since static and kinematic bodies have no finite mass.
        FUNCTION()
        float GetMass(ECS::FEntity Entity);

        //~ One body by target, such as a ragdoll limb or the body a raycast hit. A stale or unset target is a no-op.

        // The ragdoll limb simulating Bone, or its nearest ancestor with a body. Unset until the entity's ragdoll simulates.
        FUNCTION()
        SPhysicsBodyTarget GetRagdollBody(ECS::FEntity Entity, FName Bone);

        FUNCTION()
        void AddImpulseToBody(SPhysicsBodyTarget Body, FVector3 Impulse, FVector3 Position);

        FUNCTION()
        void SetBodyLinearVelocity(SPhysicsBodyTarget Body, FVector3 Velocity);

        FUNCTION()
        void SetBodyAngularVelocity(SPhysicsBodyTarget Body, FVector3 Velocity);

        FUNCTION()
        FVector3 GetBodyLinearVelocity(SPhysicsBodyTarget Body);

        FUNCTION()
        void SetGravityFactor(ECS::FEntity Entity, float Factor);

        FUNCTION()
        void ActivateBody(ECS::FEntity Entity);

        FUNCTION()
        void DeactivateBody(ECS::FEntity Entity);

        /** Awake rather than asleep. False when the entity has no body. */
        FUNCTION()
        bool IsAwake(ECS::FEntity Entity);

        /** Drags bodies resting on this collider, the conveyor belt. Zero clears it. */
        FUNCTION()
        void SetSurfaceVelocity(ECS::FEntity Entity, FVector3 Linear, FVector3 Angular);

        //~ Constraints. A body left as the null entity anchors that side to the world.

        FUNCTION()
        uint32 CreateConstraint(Physics::SConstraintDesc Desc);

        /** Welds the two bodies rigidly at their current relative pose. */
        FUNCTION()
        uint32 CreateFixedConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
            float BreakForce = 0.0f);

        /** Ball and socket pinned at Pivot, which removes the three translation degrees of freedom. */
        FUNCTION()
        uint32 CreatePointConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
            FVector3 Pivot, float BreakForce = 0.0f);

        /** Holds PointA and PointB between the two distances. A negative limit means the current distance. */
        FUNCTION()
        uint32 CreateDistanceConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
            FVector3 PointA, FVector3 PointB, float MinDistance = -1.0f, float MaxDistance = -1.0f,
            float Frequency = 0.0f, float Damping = 0.0f, float BreakForce = 0.0f);

        /** Single axis hinge at Pivot. Angles are radians and only apply when bLimited is set. */
        FUNCTION()
        uint32 CreateHingeConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
            FVector3 Pivot, FVector3 Axis, float MinAngle = 0.0f, float MaxAngle = 0.0f, bool bLimited = false,
            float MaxFrictionTorque = 0.0f, float MotorTorqueLimit = 0.0f, float MotorFrequency = 0.0f,
            float MotorDamping = 0.0f, float BreakForce = 0.0f);

        /** Prismatic slider along Axis. Distances are meters and only apply when bLimited is set. */
        FUNCTION()
        uint32 CreateSliderConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
            FVector3 Pivot, FVector3 Axis, float MinDistance = 0.0f, float MaxDistance = 0.0f,
            bool bLimited = false, float MaxFrictionForce = 0.0f, float MotorForceLimit = 0.0f,
            float MotorFrequency = 0.0f, float MotorDamping = 0.0f, float BreakForce = 0.0f);

        /** Swing limited ball and socket, keeping B's twist axis within HalfConeAngle radians of A's. */
        FUNCTION()
        uint32 CreateConeConstraint(ECS::FEntity BodyA, ECS::FEntity BodyB,
            FVector3 Pivot, FVector3 TwistAxis, float HalfConeAngle, float BreakForce = 0.0f);

        FUNCTION()
        void DestroyConstraint(uint32 ConstraintID);

        FUNCTION()
        void SetConstraintEnabled(uint32 ConstraintID, bool bEnabled);

        FUNCTION()
        void SetConstraintMotor(uint32 ConstraintID, Physics::EConstraintMotorMode Mode,
            float Target);

        FUNCTION()
        bool IsConstraintBroken(uint32 ConstraintID);

        /** A hinge reports radians and a slider meters. Every other type reports zero. */
        FUNCTION()
        float GetConstraintValue(uint32 ConstraintID);

        //~ Camera.

        // Eases from the current view over BlendTime seconds, where 0 snaps. An entity without a camera is ignored.
        FUNCTION()
        void SetActiveCamera(ECS::FEntity Entity, float BlendTime = 0.0f, ECameraBlendFunction Function = ECameraBlendFunction::EaseInOut);

        FUNCTION()
        SCameraComponent* GetActiveCamera() const;

        FUNCTION()
        ECS::FEntity GetActiveCameraEntity() const;

        // Null when nothing has resolved a view this frame.
        const FResolvedSceneView* GetResolvedView() const;

        //~ The resolved view, projections and shakes.

        // Taken from the resolved view, so an active blend and any running shake are already folded in.
        FUNCTION()
        FVector3 GetViewPosition();

        FUNCTION()
        FVector3 GetViewForward();

        FUNCTION()
        FVector3 GetViewRight();

        FUNCTION()
        FVector3 GetViewUp();

        /** Vertical field of view in degrees, zero before anything has been rendered. */
        FUNCTION()
        float GetFieldOfView();

        /** Returns a handle to stop the shake with. Several shakes sum on the rendered view. */
        FUNCTION()
        uint32 PlayCameraShake(SCameraShakeParams Params);

        /** An impact or explosion kick, where Intensity near one is a light bump and five a heavy blast. */
        FUNCTION()
        uint32 PlayImpactCameraShake(float Intensity, float Duration = 0.4f);

        FUNCTION()
        void StopCameraShake(uint32 ShakeHandle);

        FUNCTION()
        void StopAllCameraShakes();

        /** Viewport size in pixels, or zero when the world has no renderer such as a dedicated server. */
        FUNCTION()
        FVector2 GetViewportSize();

        /** Projects a world point to pixel coordinates, origin top-left. Check bOnScreen before using it. */
        FUNCTION()
        SScreenProjection WorldToScreen(FVector3 WorldLocation);

        /** Pixel coordinates to a world ray, the aiming and picking primitive. */
        FUNCTION()
        SWorldRay ScreenToWorldRay(FVector2 ScreenPosition);

        /** ScreenToWorldRay through the center of the viewport, which is where a crosshair sits. */
        FUNCTION()
        SWorldRay ViewportCenterRay();

        /** A point WorldDistance along the ray through ScreenPosition, measured from the near plane. */
        FUNCTION()
        FVector3 DeprojectScreenToWorld(FVector2 ScreenPosition, float WorldDistance);

        // On screen in the active view, within MaxDistance, and with nothing solid in between. IgnoreA is the viewer's body, IgnoreB the target's.
        FUNCTION()
        bool IsPointVisible(FVector3 Point, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB, float MaxDistance);

        // The same test from any eye, which is how a server judges what each player's camera sees.
        FUNCTION()
        bool IsPointVisibleFrom(FVector3 Eye, FVector3 Forward, float FovDegrees, float Aspect,
            FVector3 Point, ECS::FEntity IgnoreA, ECS::FEntity IgnoreB, float MaxDistance);

        //~ Rendering.

        IRenderScene* GetRenderer() const { return RenderScene.get(); }

        // Both idempotent. The lifecycle calls them, and a renderer swap such as a C# render scene reload may too.
        void CreateRenderer();
        void DestroyRenderer();

        // False headless and on a dedicated server, which stays invisible even in the editor.
        NODISCARD bool ShouldRender() const;

        // Records Callback into the world's frame at every stage in Stages, until removed.
        FRenderCallbackHandle AddRenderCallback(ERenderStage Stages, FRenderCallback Callback, FName Name = "RenderCallback")
        {
            return RenderCallbacks.Add(Stages, Move(Callback), Name);
        }

        void RemoveRenderCallback(FRenderCallbackHandle& Handle) { RenderCallbacks.Remove(Handle); }

        NODISCARD auto& GetRenderCallbacks(this auto& Self);

        // The Rml context and documents, created in InitializeWorld and freed in TeardownWorld.
        FWorldUIContext* GetUIContext() const { return UIContext.get(); }

        //~ Render target painting, queued here and run during the next render.

        void EnqueueRenderTargetPaint(FTexturePaintOp&& Op);

        // A soft radial brush at UV, where RadiusUV is relative to the longer side and Hardness above 1 sharpens it.
        FUNCTION()
        void PaintRenderTarget(CTextureRenderTarget* Target, FVector2 UV, float RadiusUV, FVector4 Color, float Strength = 1.0f, float Hardness = 1.0f, CTexture* BrushMask = nullptr);

        FUNCTION()
        void ClearRenderTarget(CTextureRenderTarget* Target, FVector4 Color);

        // The render scene's Extract moves the queued paints into its frame snapshot.
        void DrainRenderTargetPaints(TVector<FTexturePaintOp>& OutOps);

        //~ Debug drawing. A Duration at or below zero draws for one frame.

        FUNCTION()
        void DrawLine(FVector3 Start, FVector3 End, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawSphere(FVector3 Center, float Radius, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
            FVector4 Color, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawBoxCorners(FVector3 Center, FVector3 HalfExtents, FQuat Rotation,
            FVector4 Color, float CornerFraction = 0.25f, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawCapsule(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawCone(FVector3 Apex, FVector3 Direction, float AngleRadians, float Length,
            FVector4 Color, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawArrow(FVector3 Start, FVector3 Direction, float Length, FVector4 Color,
            float HeadSize = 0.2f, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawCircle(FVector3 Center, FVector3 Normal, float Radius, FVector4 Color,
            uint8 Segments = 32, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawArc(FVector3 Center, FVector3 Normal, float Radius, float StartAngle,
            float SweepAngle, FVector4 Color, uint8 Segments = 16, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawCylinder(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            uint8 Segments = 16, float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawAxes(FVector3 Origin, FQuat Rotation, float Length,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawPoint(FVector3 Position, float Size, FVector4 Color,
            float Thickness = 1.0f, float Duration = -1.0f, bool bDepthTest = true);

        FUNCTION()
        void DrawSolidTriangles(const SDebugDrawVertex* Vertices, int32 Count,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidTriangle(FVector3 A, FVector3 B, FVector3 C, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidQuad(FVector3 A, FVector3 B, FVector3 C, FVector3 D, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidBox(FVector3 Center, FVector3 HalfExtents, FQuat Rotation, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidSphere(FVector3 Center, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidCylinder(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidCone(FVector3 Apex, FVector3 Direction, float AngleRadians, float Length,
            FVector4 Color, EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidCapsule(FVector3 Start, FVector3 End, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidCircle(FVector3 Center, FVector3 Normal, float Radius, FVector4 Color,
            EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidArc(FVector3 Center, FVector3 Normal, float Radius, float StartAngle,
            float SweepAngle, FVector4 Color, EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        FUNCTION()
        void DrawSolidArrow(FVector3 Start, FVector3 Direction, float Length, FVector4 Color,
            float HeadSize = 0.2f, EDebugFillMode Mode = EDebugFillMode::Translucent, float Duration = -1.0f);

        /** One screen-space line, stacked from the top left of the world viewport for this frame only. */
        FUNCTION()
        void DrawText(const FString& Text, FVector4 Color);

        NODISCARD FWorldDebugInterface* GetDebugInterface() { return &DebugInterface; }

        void DrawBillboard(int32 ResourceID, const FVector3& Location, float Scale);

        // Immediate line sink for large single-frame batches, when the world has a renderer.
        FImmediateLineRenderer* GetImmediateLines() const;

        // The render scene moves the queued text lines out each frame.
        void DrainDebugTextLines(TVector<FDebugTextLine>& Out);

        //~ Settings and scene folders.

        SDefaultWorldSettings& GetDefaultWorldSettings();

        // The outliner's folder table, created on first use.
        SSceneFolderComponent& GetSceneFolders();

        // Null when the world has never had a folder table.
        NODISCARD auto* FindSceneFolders(this auto& Self);

        //~ Reflected state, public so the hot reload reinstancer and tools reach it.

        // One instance of every enabled system class, which the per-stage slots point into.
        PROPERTY(Hidden, NoSerialize)
        TVector<TStrongObjectPtr<CEntitySystem>> Systems;

        PROPERTY(Hidden, NoSerialize)
        TVector<TStrongObjectPtr<CWorldSubsystem>> Subsystems;

    private:

        //~ System scheduling.

        void TickSystems(SSystemContext& Context);

        // Rebuilds the per-stage slot lists and their parallel batches from Systems.
        void RebuildSystemSchedule();

        // Runs OnStartup on any system that has not had it, once the world's own startup pass has run.
        void StartupPendingSystems();

        // Applies a SetSystemEnabled request at the top of Update, so it never runs inside a batch.
        void ApplyPendingSystemChanges();

        // Latches whether this frame runs, from FWorldManager::BeginFrame.
        void AdvanceThrottle(double NowSeconds);

        //~ Registry signal handlers, connected in InitializeWorld.

        void OnTransformComponentConstruct(ECS::FRegistry& Registry, ECS::FEntity Entity);
        void OnCSharpScriptComponentDestroyed(ECS::FRegistry& Registry, ECS::FEntity Entity);
        void OnWidgetComponentDestroyed(ECS::FRegistry& Registry, ECS::FEntity Entity);

        // Logs Caller when Entity has no transform, for the script-facing transform setters.
        STransformComponent* FindScriptTransform(ECS::FEntity Entity, const char* Caller);

        //~ Entities and the buses that record against them, in construction order.

        ECS::FRegistry          RegistryPending;
        ECS::FRegistry          EntityRegistry;
        ECS::FEntity            SingletonEntity;

        // After the registry it records against, before the context that hands it out.
        ECS::FCommandBus        CommandBus;

        // After the registry its fan-outs deliver into, before the context that hands it out.
        ECS::FEventQueues       EventQueues;

        // After the command bus, since a send from a parallel system is replayed at its flush.
        FGameplayMessageBus     MessageBus;

        SSystemContext          SystemContext;

        //~ Owned scenes.

        TUniquePtr<IRenderScene>            RenderScene;
        FRenderCallbackList                 RenderCallbacks;
        TUniquePtr<Physics::IPhysicsScene>  PhysicsScene;
        TUniquePtr<FWorldUIContext>         UIContext;

        //~ System schedule.

        // Per stage, the priority-sorted slots TickSystems runs.
        TVector<FStageSlot>     SystemUpdateList[(int32)EUpdateStage::Max];

        // Which slots with work may run together, rebuilt only when that set changes between frames.
        TVector<FSystemBatch>   SystemBatches[(int32)EUpdateStage::Max];

        // Per slot, whether it had work when SystemBatches was built. Empty forces a rebuild.
        TVector<uint8>          SystemsWithWork[(int32)EUpdateStage::Max];

        // Set once the world has run its startup pass, which gates StartupPendingSystems.
        bool                    bSystemsStarted = false;

        // The C# script generation the scripted systems were built against, so a reload rebuilds them.
        int32                   ScriptGeneration = -1;

        // NetRealm::KeyOf when the systems were last registered.
        uint32                  RegisteredRealmKey = 0;

        // The applied and the requested disabled systems, which differ only until ApplyPendingSystemChanges.
        THashSet<FName>         DisabledSystems;
        THashSet<FName>         PendingDisabledSystems;
        bool                    bSystemsDirty = false;

        //~ Debug drawing and render target painting.

        FLineBatcherComponent*              LineBatcherComponent;
        FTriangleBatcherComponent*          TriangleBatcherComponent;
        TVector<FDebugTextLine>             DebugTextLines;
        TBoundedMPSCQueue<FTexturePaintOp>  RenderTargetPaintQueue;

        // Bound under World.Debug, with its World pointing back here.
        FWorldDebugInterface                DebugInterface;

        //~ Context, time and throttling.

        FWorldContext*          OwningContext = nullptr;

        double                  DeltaTime = 0.0;
        double                  RealDeltaTime = 0.0;
        double                  TimeSinceCreation = 0.0;

        // Engine clock time the world went suspended, or -1 while active, for the idle reclaim grace.
        double                  SuspendedTime = -1.0;

        // Interval 0 runs every frame, and NextUpdateTime is the engine clock deadline for the next one.
        double                  UpdateIntervalSeconds = 0.0;
        double                  NextUpdateTime = 0.0;

        uint32                  bPaused:1 = true;
        uint32                  bAnimatesWhilePaused:1 = true;
        uint32                  bActive:1 = true;
        uint32                  bThrottledThisFrame:1 = false;

        EWorldType              WorldType = EWorldType::None;
        bool                    bInitializing = true;
        bool                    bTearingDown = false;
    };

    // The scheduling CWorld::TickSystems runs, as free functions so it can be tested without a world.
    namespace SystemSchedule
    {
        // Priority first, then exclusive systems last, then the class name, so equal priorities order the same every run.
        RUNTIME_API void SortStage(TVector<CWorld::FStageSlot>& Stage);

        // A slot lands in the first batch after the last one holding anything it conflicts with, and slots without work are left out.
        RUNTIME_API TVector<CWorld::FSystemBatch> BuildBatches(const TVector<CWorld::FStageSlot>& Stage, const TVector<uint8>& WithWork);
    }
}

#include "World.inl"
