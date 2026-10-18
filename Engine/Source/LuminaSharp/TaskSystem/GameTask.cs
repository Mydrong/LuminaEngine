using System;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;

using Lumina;

namespace LuminaSharp;

// Game thread awaits scheduled on the world's coroutine scheduler, canceled with the calling script unless a token says otherwise.
public static class GameTask
{
    // Registered so a generation unload or its world's teardown completes it, rather than leaving the await site never to resume.
    private sealed class FPending<T> : GameTaskRegistry.ICancelPending
    {
        internal readonly TaskCompletionSource<T> Source = new();

        // The callback context at the await, so the continuation sees the same world, entity and script.
        internal readonly Engine.Scope Context = Engine.Snapshot();

        private CancellationTokenRegistration Registration;
        private Lumina.CWorld? WaitWorld;
        private ulong Wait;
        private int bDone;

        public ulong World { get; }

        internal FPending(Lumina.CWorld? InWorld)
        {
            World = InWorld != null ? InWorld.WorldHandle : 0;
            GameTaskRegistry.Track(this);
        }

        internal void WaitOn(Lumina.CWorld InWorld, ulong InWait)
        {
            WaitWorld = InWorld;
            Wait = InWait;
        }

        internal void Settle(T Value)
        {
            if (Finish())
            {
                using (Engine.Resume(Context))
                {
                    Source.TrySetResult(Value);
                }
            }
        }

        public void Cancel()
        {
            if (!Finish())
            {
                return;
            }

            // A torn-down world has already dropped its waits, and reaching for one there would touch a dead world.
            if (WaitWorld != null && Wait != 0 && WaitWorld.IsValid)
            {
                WaitWorld.CancelWait(Wait);
            }
            Source.TrySetCanceled();
        }

        // The world dropped the wait itself, so there is nothing native left to cancel.
        internal void Abandon()
        {
            if (Finish())
            {
                Source.TrySetCanceled();
            }
        }

        // Explicit tokens win, and otherwise the calling script's lifetime bounds the await.
        internal void CancelOn(CancellationToken Token)
        {
            if (!Token.CanBeCanceled && Engine.ActiveScript is { } Owner)
            {
                Token = Owner.LifetimeToken;
            }
            if (Token.CanBeCanceled)
            {
                Registration = Token.Register(Cancel);
            }
        }

        // Exactly one of Settle and Cancel wins, and the winner drops the registration so a per-frame await cannot pile them up.
        private bool Finish()
        {
            if (Interlocked.Exchange(ref bDone, 1) != 0)
            {
                return false;
            }
            GameTaskRegistry.Forget(this);
            Registration.Dispose();
            return true;
        }
    }

    private static System.Threading.Tasks.Task Schedule(CancellationToken Token, Func<Lumina.CWorld, SScriptCallback, ulong> Start)
    {
        if (!Engine.InWorld)
        {
            FPending<bool> Orphan = new(null);
            Orphan.Cancel();
            return Orphan.Source.Task;
        }

        Lumina.CWorld World = Engine.World;
        FPending<bool> Pending = new(World);
        Pending.CancelOn(Token);
        ulong Wait = Start(World, ScriptCallback.OfInt(Completed =>
        {
            if (Completed != 0)
            {
                Pending.Settle(true);
            }
            else
            {
                Pending.Abandon();
            }
        }));
        Pending.WaitOn(World, Wait);
        return Pending.Source.Task;
    }

    // Counts unpaused frames only, like the native Coro::Ticks.
    public static System.Threading.Tasks.Task Ticks(int Count, CancellationToken Token = default)
        => Schedule(Token, (World, Done) => World.WaitTicks(Count, Done));

    public static System.Threading.Tasks.Task NextFrame(CancellationToken Token = default) => Ticks(1, Token);

    public static System.Threading.Tasks.Task Seconds(double Duration, ECoroutineClock Clock = ECoroutineClock.World, CancellationToken Token = default)
        => Schedule(Token, (World, Done) => World.WaitSeconds(Duration, Clock, Done));

    public static System.Threading.Tasks.Task DelaySeconds(float Duration, CancellationToken Token = default) => Seconds(Duration, ECoroutineClock.World, Token);

    public static System.Threading.Tasks.Task UnpausedSeconds(double Duration, CancellationToken Token = default) => Seconds(Duration, ECoroutineClock.Unpaused, Token);

    public static System.Threading.Tasks.Task RealSeconds(double Duration, CancellationToken Token = default) => Seconds(Duration, ECoroutineClock.Real, Token);

    // Checked at once and then once a frame, resuming on the first true.
    public static async System.Threading.Tasks.Task Until(Func<bool> Predicate, CancellationToken Token = default)
    {
        while (!Predicate())
        {
            await NextFrame(Token);
        }
    }

    // Starts every racer on one shared token, returns the first to end and cancels the rest through that token.
    public static async System.Threading.Tasks.Task<int> Race(CancellationToken Token, params Func<CancellationToken, System.Threading.Tasks.Task>[] Racers)
    {
        if (!Token.CanBeCanceled && Engine.ActiveScript is { } Owner)
        {
            Token = Owner.LifetimeToken;
        }
        using CancellationTokenSource Losers = CancellationTokenSource.CreateLinkedTokenSource(Token);
        System.Threading.Tasks.Task[] Running = new System.Threading.Tasks.Task[Racers.Length];
        for (int Index = 0; Index < Racers.Length; ++Index)
        {
            Running[Index] = Racers[Index](Losers.Token);
        }
        System.Threading.Tasks.Task Winner = await System.Threading.Tasks.Task.WhenAny(Running);
        Losers.Cancel();
        return Array.IndexOf(Running, Winner);
    }

    public static System.Threading.Tasks.Task<int> Race(params Func<CancellationToken, System.Threading.Tasks.Task>[] Racers) => Race(default, Racers);

    // Continues on a thread pool thread until MoveToGameThread, so nothing after it may touch the world.
    public static ThreadPoolSwitch MoveToTask() => default;

    public static GameThreadSwitch MoveToGameThread() => default;

    // Resumes once everything the handle asked for has loaded, which is what awaiting an FAssetHandle does.
    public static System.Threading.Tasks.Task<FAssetHandle> WhenReady(FAssetHandle Handle, CancellationToken Token = default)
    {
        FPending<FAssetHandle> Pending = new(Engine.InWorld ? Engine.World : null);
        Pending.CancelOn(Token);
        Handle.OnReady(Loaded => Pending.Settle(Loaded));
        return Pending.Source.Task;
    }

    // The loaded handle, which keeps what it loaded resident until disposed. Invalid when the path resolves to nothing.
    public static System.Threading.Tasks.Task<FAssetHandle> LoadAsync(string Path, string Bundle = "", CancellationToken Token = default)
        => WhenReady(FAssetManager.Load(Path, Bundle), Token);

    public static System.Threading.Tasks.Task<FAssetHandle> LoadByNameAsync(string Group, string Name, string Bundle = "", CancellationToken Token = default)
        => WhenReady(FAssetManager.LoadByName(Group, Name, Bundle), Token);

    public static System.Threading.Tasks.Task<FAssetHandle> LoadGroupAsync(string Group, string Bundle = "", CancellationToken Token = default)
        => WhenReady(FAssetManager.LoadGroup(Group, Bundle), Token);

    // Loads without blocking and resumes on the game thread with the object, or null when it failed or is not a T.
    public static async System.Threading.Tasks.Task<T?> LoadObjectAsync<T>(string Path, CancellationToken Token = default) where T : NativeObject
    {
        using FAssetHandle Handle = FAssetManager.Load(Path);
        return (await WhenReady(Handle, Token)).Get<T>();
    }

    public static System.Threading.Tasks.Task<T?> LoadObjectAsync<T>(FSoftObjectPath Path, CancellationToken Token = default) where T : NativeObject
        => LoadObjectAsync<T>(Path.GetPath(), Token);

    public static System.Threading.Tasks.Task<T?> LoadObjectAsync<T>(TSoftObjectPtr<T> Object, CancellationToken Token = default) where T : NativeObject
        => LoadObjectAsync<T>(Object.GetPath(), Token);

    public readonly struct ThreadPoolSwitch : ICriticalNotifyCompletion
    {
        public ThreadPoolSwitch GetAwaiter() => this;

        public bool IsCompleted => false;

        public void GetResult()
        {
        }

        public void OnCompleted(Action Continuation) => UnsafeOnCompleted(Continuation);

        // The scope rides along, so the MoveToGameThread that ends the hop resumes with the same world and script.
        public void UnsafeOnCompleted(Action Continuation)
        {
            ThreadPool.UnsafeQueueUserWorkItem(static State =>
            {
                (Action Work, Engine.Scope Context) = ((Action, Engine.Scope))State!;
                using (Engine.Resume(Context))
                {
                    Work();
                }
            }, (Continuation, Engine.Snapshot()));
        }
    }

    public readonly struct GameThreadSwitch : ICriticalNotifyCompletion
    {
        public GameThreadSwitch GetAwaiter() => this;

        public bool IsCompleted => GameThreadContext.OnGameThread;

        public void GetResult()
        {
        }

        public void OnCompleted(Action Continuation) => GameThreadContext.Run(Continuation);

        public void UnsafeOnCompleted(Action Continuation) => GameThreadContext.Run(Continuation);
    }
}
