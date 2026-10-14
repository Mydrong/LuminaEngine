namespace Lumina;

// Handwritten flag readers over the reflected SInputActionState, matching its native EFlags bits.
public partial struct SInputActionState
{
    public bool IsDown => (Flags & 1u) != 0;
    public bool IsPressed => (Flags & 2u) != 0;
    public bool IsReleased => (Flags & 4u) != 0;
    public bool IsHeld => (Flags & 8u) != 0;
    public bool IsTapped => (Flags & 16u) != 0;
}
