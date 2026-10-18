using Lumina;

namespace LuminaSharp;

/// A live joint connecting two rigid bodies, or a body to the world, created by the World.Create*Constraint calls. A lightweight value of world plus native id, safe to copy and store. Game thread only.
public readonly struct FPhysicsConstraint
{
    private readonly CWorld World;

    /// Opaque native handle. 0 means creation failed, such as when neither side had a rigid body.
    public readonly uint Id;

    public FPhysicsConstraint(CWorld World, uint Id)
    {
        this.World = World;
        this.Id = Id;
    }

    /// False when the joint failed to create. Operations on an invalid handle are safe no-ops.
    public bool IsValid => Id != 0;

    /// Removes the joint from the simulation. The handle is invalid afterwards.
    public void Destroy()
    {
        if (Id != 0)
        {
            World.DestroyConstraint(Id);
        }
    }

    /// Enable or disable the joint without destroying it. Re-enabling clears a broken flag.
    public void SetEnabled(bool Enabled)
    {
        if (Id != 0)
        {
            World.SetConstraintEnabled(Id, Enabled);
        }
    }

    /// Drives a hinge or slider motor toward a target velocity, rad/s for a hinge and m/s for a slider.
    public void DriveToVelocity(float Target)
    {
        if (Id != 0)
        {
            World.SetConstraintMotor(Id, Lumina.Physics.EConstraintMotorMode.Velocity, Target);
        }
    }

    /// Drives a hinge or slider motor toward a target position through its motor spring.
    public void DriveToPosition(float Target)
    {
        if (Id != 0)
        {
            World.SetConstraintMotor(Id, Lumina.Physics.EConstraintMotorMode.Position, Target);
        }
    }

    /// Turns the motor off, leaving the joint free or friction only.
    public void DisableMotor()
    {
        if (Id != 0)
        {
            World.SetConstraintMotor(Id, Lumina.Physics.EConstraintMotorMode.Off, 0.0f);
        }
    }

    /// True once a breakable joint exceeded its break force and was auto-disabled.
    public bool IsBroken => Id != 0 && World.IsConstraintBroken(Id);

    /// A hinge's angle in radians or a slider's position in meters. Zero for the types without one.
    public float CurrentValue => Id != 0 ? World.GetConstraintValue(Id) : 0.0f;
}
