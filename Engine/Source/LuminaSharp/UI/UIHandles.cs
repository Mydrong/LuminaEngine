namespace Lumina;

// Hand-written halves of the Reflector's UI handle mirrors, so validity reads off the type itself.

public partial struct SUIDocument
{
    public bool IsValid => Handle != 0;
}

public partial struct SUIElement
{
    public bool IsValid => Handle != 0;
}

public partial struct SUIEventListener
{
    public bool IsValid => Handle != 0;
}

public partial struct SUIDataModel
{
    public bool IsValid => Handle != 0;
}
