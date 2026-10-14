using System;

// Rider and ReSharper match these by full name, so the engine's attributes can say what the engine uses without a package.
namespace JetBrains.Annotations;

[Flags]
internal enum ImplicitUseKindFlags
{
    Access = 1,
    Assign = 2,
    InstantiatedWithFixedConstructorSignature = 4,
    InstantiatedNoFixedConstructorSignature = 8,
    Default = Access | Assign | InstantiatedWithFixedConstructorSignature,
}

[Flags]
internal enum ImplicitUseTargetFlags
{
    Itself = 1,
    Members = 2,
    WithInheritors = 4,
    Default = Itself,
    WithMembers = Itself | Members,
}

[AttributeUsage(AttributeTargets.Class | AttributeTargets.GenericParameter | AttributeTargets.Parameter)]
internal sealed class MeansImplicitUseAttribute : Attribute
{
    public MeansImplicitUseAttribute() : this(ImplicitUseKindFlags.Default, ImplicitUseTargetFlags.Default) { }
    public MeansImplicitUseAttribute(ImplicitUseKindFlags UseKindFlags) : this(UseKindFlags, ImplicitUseTargetFlags.Default) { }
    public MeansImplicitUseAttribute(ImplicitUseTargetFlags TargetFlags) : this(ImplicitUseKindFlags.Default, TargetFlags) { }

    public MeansImplicitUseAttribute(ImplicitUseKindFlags UseKindFlags, ImplicitUseTargetFlags TargetFlags)
    {
        this.UseKindFlags = UseKindFlags;
        this.TargetFlags = TargetFlags;
    }

    public ImplicitUseKindFlags UseKindFlags { get; }
    public ImplicitUseTargetFlags TargetFlags { get; }
}
