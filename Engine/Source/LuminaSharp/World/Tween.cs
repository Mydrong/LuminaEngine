using System;
using LuminaSharp;

namespace Lumina;

// A fluent handle over one native tween, the C# side of FTween. Copyable, and safe to keep after the tween ends.
public readonly struct Tween : IEquatable<Tween>
{
    private readonly CWorld World;
    private readonly uint Id;

    internal Tween(CWorld World, uint Id)
    {
        this.World = World;
        this.Id = Id;
    }

    //~ Tweeners. Each starts a new sequential step unless Parallel() preceded it.

    public Tween MoveTo(FEntity Target, FVector3 Destination, float Duration)
    {
        World.Tweens.MoveTo(Id, Target, Destination, Duration);
        return this;
    }

    public Tween RotateTo(FEntity Target, FQuat Destination, float Duration)
    {
        World.Tweens.RotateTo(Id, Target, Destination, Duration);
        return this;
    }

    public Tween ScaleTo(FEntity Target, FVector3 Destination, float Duration)
    {
        World.Tweens.ScaleTo(Id, Target, Destination, Duration);
        return this;
    }

    // Relative to wherever Target is when the step begins.
    public Tween MoveBy(FEntity Target, FVector3 Offset, float Duration)
    {
        World.Tweens.MoveBy(Id, Target, Offset, Duration);
        return this;
    }

    public Tween RotateBy(FEntity Target, FQuat Delta, float Duration)
    {
        World.Tweens.RotateBy(Id, Target, Delta, Duration);
        return this;
    }

    // Multiplies the starting scale, so a factor of two doubles it.
    public Tween ScaleBy(FEntity Target, FVector3 Factor, float Duration)
    {
        World.Tweens.ScaleBy(Id, Target, Factor, Duration);
        return this;
    }

    // A kick toward Amount that swings back and settles, rotation in degrees. Vibrato counts the swings.
    public Tween Punch(FEntity Target, ETweenTransformChannel Channel, FVector3 Amount, float Duration, int Vibrato = 10, float Elasticity = 1.0f)
    {
        World.Tweens.Punch(Id, Target, Channel, Amount, Duration, Vibrato, Elasticity);
        return this;
    }

    // A decaying random wobble of up to Strength on each axis, rotation in degrees.
    public Tween Shake(FEntity Target, ETweenTransformChannel Channel, FVector3 Strength, float Duration, int Vibrato = 10)
    {
        World.Tweens.Shake(Id, Target, Channel, Strength, Duration, Vibrato);
        return this;
    }

    public Tween Value(float From, float To, float Duration, Action<float> OnValue)
    {
        World.Tweens.ValueTo(Id, From, To, Duration, ScriptCallback.OfFloat(OnValue));
        return this;
    }

    public Tween Value(FVector2 From, FVector2 To, float Duration, Action<FVector2> OnValue)
    {
        World.Tweens.ValueToVector2(Id, From, To, Duration, ScriptCallback.Bind(new ValueClosure<FVector2>(OnValue)));
        return this;
    }

    public Tween Value(FVector3 From, FVector3 To, float Duration, Action<FVector3> OnValue)
    {
        World.Tweens.ValueToVector3(Id, From, To, Duration, ScriptCallback.Bind(new ValueClosure<FVector3>(OnValue)));
        return this;
    }

    public Tween Value(FVector4 From, FVector4 To, float Duration, Action<FVector4> OnValue)
    {
        World.Tweens.ValueToVector4(Id, From, To, Duration, ScriptCallback.Bind(new ValueClosure<FVector4>(OnValue)));
        return this;
    }

    // Slerps, so a wide turn follows the arc.
    public Tween Value(FQuat From, FQuat To, float Duration, Action<FQuat> OnValue)
    {
        World.Tweens.ValueToQuat(Id, From, To, Duration, ScriptCallback.Bind(new ValueClosure<FQuat>(OnValue)));
        return this;
    }

    // Drives a reflected property of Target's T by a dotted path such as "Settings.Intensity", natively with no call per frame.
    public Tween Property<T>(FEntity Target, string Path, float To, float Duration) where T : struct, IComponent<T>
    {
        World.Tweens.PropertyToFloat(Id, Target, T.NativeName, Path, To, Duration);
        return this;
    }

    public Tween Property<T>(FEntity Target, string Path, FVector2 To, float Duration) where T : struct, IComponent<T>
    {
        World.Tweens.PropertyToVector2(Id, Target, T.NativeName, Path, To, Duration);
        return this;
    }

    public Tween Property<T>(FEntity Target, string Path, FVector3 To, float Duration) where T : struct, IComponent<T>
    {
        World.Tweens.PropertyToVector3(Id, Target, T.NativeName, Path, To, Duration);
        return this;
    }

    public Tween Property<T>(FEntity Target, string Path, FVector4 To, float Duration) where T : struct, IComponent<T>
    {
        World.Tweens.PropertyToVector4(Id, Target, T.NativeName, Path, To, Duration);
        return this;
    }

    public Tween Property<T>(FEntity Target, string Path, FQuat To, float Duration) where T : struct, IComponent<T>
    {
        World.Tweens.PropertyToQuat(Id, Target, T.NativeName, Path, To, Duration);
        return this;
    }

    // Dead time, for spacing steps apart.
    public Tween Interval(float Duration)
    {
        World.Tweens.Interval(Id, Duration);
        return this;
    }

    // Runs once when the sequence reaches this step.
    public Tween Call(Action OnCall)
    {
        World.Tweens.Call(Id, ScriptCallback.Of(OnCall));
        return this;
    }

    //~ Modifiers. Trans, Ease, EaseCurve and Delay apply to the step added last.

    public Tween Trans(EEaseTransition Transition)
    {
        World.Tweens.Trans(Id, Transition);
        return this;
    }

    public Tween Ease(EEaseType Ease)
    {
        World.Tweens.Ease(Id, Ease);
        return this;
    }

    // Shapes the step with the curve over its keyed time range, replacing Trans and Ease.
    public Tween EaseCurve(CCurveAsset Curve)
    {
        World.Tweens.EaseCurve(Id, Curve);
        return this;
    }

    public Tween Delay(float Seconds)
    {
        World.Tweens.Delay(Id, Seconds);
        return this;
    }

    // Runs the next tweener alongside the last one rather than after it.
    public Tween Parallel()
    {
        World.Tweens.Parallel(Id);
        return this;
    }

    // Zero or less loops forever.
    public Tween SetLoops(int Count, ETweenLoopMode Mode = ETweenLoopMode.Restart)
    {
        World.Tweens.SetLoops(Id, Count);
        World.Tweens.SetLoopMode(Id, Mode);
        return this;
    }

    public Tween SetSpeedScale(float Scale)
    {
        World.Tweens.SetSpeedScale(Id, Scale);
        return this;
    }

    // Runs on real time, through world pause and time dilation, for menus and other UI.
    public Tween SetIgnoreTimeScale(bool Ignore = true)
    {
        World.Tweens.SetIgnoreTimeScale(Id, Ignore);
        return this;
    }

    //~ Events.

    public Tween OnFinished(Action Handler)
    {
        World.Tweens.OnFinished(Id, ScriptCallback.Of(Handler));
        return this;
    }

    // Runs as each pass ends and another begins, with the passes completed so far.
    public Tween OnLoop(Action<int> Handler)
    {
        World.Tweens.OnLoop(Id, ScriptCallback.OfInt(Handler));
        return this;
    }

    // Runs as each step ends, with that step's index in the order it was built.
    public Tween OnStep(Action<int> Handler)
    {
        World.Tweens.OnStep(Id, ScriptCallback.OfInt(Handler));
        return this;
    }

    //~ Control and state.

    public void Pause() => World.Tweens.SetPaused(Id, true);

    public void Resume() => World.Tweens.SetPaused(Id, false);

    // Stops where it is, leaving whatever it drove at its current value.
    public void Kill() => World.Tweens.Kill(Id);

    // Jumps to the end of the current pass, running every step and OnFinished on the way.
    public void Complete() => World.Tweens.Complete(Id);

    public bool IsRunning => World != null && World.Tweens.IsRunning(Id);

    // Awaiting a tween resumes the frame it stops running, whether it finished or was killed, without taking its OnFinished.
    public System.Runtime.CompilerServices.TaskAwaiter GetAwaiter()
    {
        Tween Self = this;
        return GameTask.Until(() => !Self.IsRunning).GetAwaiter();
    }

    // Seconds into the current pass.
    public float Elapsed => World != null ? World.Tweens.GetElapsed(Id) : 0.0f;

    // The length of one pass.
    public float Duration => World != null ? World.Tweens.GetDuration(Id) : 0.0f;

    public float Progress => World != null ? World.Tweens.GetProgress(Id) : 0.0f;

    public int CompletedLoops => World != null ? World.Tweens.GetCompletedLoops(Id) : 0;

    public bool Equals(Tween Other) => Id == Other.Id && ReferenceEquals(World, Other.World);

    public override bool Equals(object? Other) => Other is Tween Value && Equals(Value);

    public override int GetHashCode() => HashCode.Combine(World, Id);

    public static bool operator ==(Tween A, Tween B) => A.Equals(B);

    public static bool operator !=(Tween A, Tween B) => !A.Equals(B);

    // Native hands over the value's address for the length of the call.
    private sealed unsafe class ValueClosure<T> : ScriptClosure where T : unmanaged
    {
        private readonly Action<T> Handler;

        public ValueClosure(Action<T> Handler)
        {
            this.Handler = Handler;
        }

        public override ulong Invoke(ulong Payload)
        {
            if (Payload != 0)
            {
                Handler(*(T*)Payload);
            }
            return 0;
        }
    }
}

public partial class CTweenSubsystem
{
    // Starts an empty tween. With an Owner, the tween dies with that entity, which is what gameplay usually wants.
    public Tween Create(FEntity? Owner = null) => new(GetWorld(), CreateId(Owner ?? FEntity.Null));
}
