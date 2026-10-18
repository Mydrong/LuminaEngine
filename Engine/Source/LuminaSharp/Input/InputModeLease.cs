using System;
using Lumina;

namespace LuminaSharp;

// One holder's request for input and mouse modes, combined with every other open request so releasing it never undoes another panel's.
public sealed class InputModeLease : IDisposable
{
    private readonly CWorld World;
    private readonly EInputMode InputMode;
    private readonly EMouseMode MouseMode;
    private ulong Lease;

    // UI mode by default, so look and move actions stop turning the camera under an open panel.
    public InputModeLease(CWorld World, EInputMode InputMode = EInputMode.UI, EMouseMode MouseMode = EMouseMode.Normal)
    {
        this.World = World;
        this.InputMode = InputMode;
        this.MouseMode = MouseMode;
    }

    // Set while a panel is open and cleared when it closes, any number of times.
    public bool Held
    {
        get => Lease != 0;
        set
        {
            if (value == Held)
            {
                return;
            }
            if (value)
            {
                Lease = World.AcquireInputMode(InputMode, MouseMode);
                return;
            }

            // A torn-down world has already dropped every lease it held.
            if (World.IsValid)
            {
                World.ReleaseInputMode(Lease);
            }
            Lease = 0;
        }
    }

    public void Dispose() => Held = false;
}
