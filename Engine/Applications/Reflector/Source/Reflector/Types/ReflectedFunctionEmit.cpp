#include "ReflectedType.h"

#include <algorithm>

#include "Reflector/CodeGeneration/CodeWriter.h"
#include "Reflector/CodeGeneration/ReflectionNames.h"

namespace Lumina::Reflection
{
    namespace
    {
        // The real class name, since a call has to name the type rather than whatever alias reflects it.
        std::string QualifiedOwner(const FReflectedStruct& Owner)
        {
            return Owner.EmittedCppQualifiedName();
        }
    }

    bool FReflectedStruct::HasNativeFunctions() const
    {
        for (const auto& Function : Functions)
        {
            if (Function->bReflectNatively)
            {
                return true;
            }
        }
        return false;
    }

    void FReflectedStruct::EmitFunctionFrames(FCodeWriter& Writer) const
    {
        if (!HasNativeFunctions())
        {
            return;
        }

        for (const auto& Function : Functions)
        {
            if (!Function->bReflectNatively)
            {
                continue;
            }

            const std::string Parms = Names::FunctionParmsStruct(DisplayName, Function->Name);
            const std::string Thunk = Names::FunctionThunk(DisplayName, Function->Name);

            Writer.Linef("struct %s", Parms.c_str());
            Writer.Line("{");
            for (size_t Index = 0; Index < Function->TopLevelParams.size(); ++Index)
            {
                Writer.Linef("\t%s %s;", Function->ParameterStorageTypes[Index].c_str(),
                    Function->TopLevelParams[Index]->Name.c_str());
            }
            Writer.Line("};");
            Writer.Line();

            // The call itself: a static takes no object, and anything else dispatches through the pointer,
            // which is what makes a virtual reach its override.
            std::string Arguments;
            const size_t NumArguments = Function->ReturnIndex >= 0
                ? Function->TopLevelParams.size() - 1
                : Function->TopLevelParams.size();

            for (size_t Index = 0; Index < NumArguments; ++Index)
            {
                if (Index != 0)
                {
                    Arguments += ", ";
                }

                // An object argument is held as a handle, so the call takes the pointer back out of it.
                // The cast is unqualified because a generated file need not have the class's definition,
                // and every reflected class shares one root, so the pointer value is the same either way.
                const std::string& CastType = Function->ParameterObjectCastTypes[Index];
                if (!CastType.empty())
                {
                    Arguments += "(" + CastType + ")P." + Function->TopLevelParams[Index]->Name + ".Get()";
                }
                else if (Function->ParameterIsRvalue[Index])
                {
                    Arguments += "std::move(P." + Function->TopLevelParams[Index]->Name + ")";
                }
                else
                {
                    Arguments += "P.";
                    Arguments += Function->TopLevelParams[Index]->Name;
                }
            }

            const std::string Owner = QualifiedOwner(*this);
            const std::string Call = Function->bIsStatic
                ? (Owner + "::" + Function->Name + "(" + Arguments + ")")
                : ("((" + Owner + "*)Context)->" + Function->Name + "(" + Arguments + ")");

            Writer.Linef("static void %s(const Lumina::FFunction&, void* Context, void* Frame)", Thunk.c_str());
            Writer.Line("{");
            if (!Function->TopLevelParams.empty())
            {
                Writer.Linef("\t%s& P = *(%s*)Frame;", Parms.c_str(), Parms.c_str());
            }
            else
            {
                Writer.Line("\t(void)Frame;");
            }
            if (Function->bIsStatic)
            {
                Writer.Line("\t(void)Context;");
            }
            if (Function->ReturnIndex >= 0)
            {
                const size_t ReturnAt = (size_t)Function->ReturnIndex;

                // An object return goes into a handle to CObject, and the derived-to-base conversion is not
                // available where the class is only forward declared, so it is spelled out.
                const std::string Assigned = Function->ParameterObjectCastTypes[ReturnAt].empty()
                    ? Call
                    : ("(Lumina::CObject*)(" + Call + ")");

                Writer.Linef("\tP.%s = %s;",
                    Function->TopLevelParams[ReturnAt]->Name.c_str(), Assigned.c_str());
            }
            else
            {
                Writer.Linef("\t%s;", Call.c_str());
            }
            Writer.Line("}");
            Writer.Line();
        }
    }

    void FReflectedStruct::EmitFunctionTable(FCodeWriter& Writer) const
    {
        if (!HasNativeFunctions())
        {
            return;
        }

        for (const auto& Function : Functions)
        {
            if (!Function->bReflectNatively || Function->TopLevelParams.empty())
            {
                continue;
            }

            Writer.Linef("static const Lumina::FPropertyDesc %s[%zu];", Names::FunctionParamsTable(Function->Name).c_str(), Function->TopLevelParams.size());
            Writer.Line();
        }

        const size_t Count = std::count_if(Functions.begin(), Functions.end(), [](const auto& Function) { return Function->bReflectNatively; });
        Writer.Linef("static const Lumina::FFunctionDesc Functions[%zu];", Count);
        Writer.Line();
    }

    void FReflectedStruct::DefineFunctionTable(FCodeWriter& Writer) const
    {
        if (!HasNativeFunctions())
        {
            return;
        }

        Writer.Linef("constexpr Lumina::FFunctionDesc %s::Functions[] =", TypeInfoName().c_str());
        Writer.BeginBlock();
        for (const auto& Function : Functions)
        {
            if (!Function->bReflectNatively)
            {
                continue;
            }

            std::string Flags = "Lumina::EFunctionFlags::None";
            if (Function->bIsStatic)  { Flags += " | Lumina::EFunctionFlags::Static"; }
            if (Function->bIsConst)   { Flags += " | Lumina::EFunctionFlags::Const"; }
            if (Function->bIsVirtual) { Flags += " | Lumina::EFunctionFlags::Virtual"; }
            for (const FMetadataPair& Pair : Function->Metadata)
            {
                if (Pair.Key == "Rpc")
                {
                    if (Pair.Value == "Broadcast")  { Flags += " | Lumina::EFunctionFlags::RpcBroadcast"; }
                    else if (Pair.Value == "Host")  { Flags += " | Lumina::EFunctionFlags::RpcHost"; }
                    else if (Pair.Value == "Owner") { Flags += " | Lumina::EFunctionFlags::RpcOwner"; }
                }
                else if (Pair.Key == "Bind")
                {
                    Flags += " | Lumina::EFunctionFlags::UIBind";
                }
                else if (Pair.Key == "NetFlags")
                {
                    if (Pair.Value.find("Unreliable") != std::string::npos) { Flags += " | Lumina::EFunctionFlags::NetUnreliable"; }
                    if (Pair.Value.find("OwnerOnly") != std::string::npos)  { Flags += " | Lumina::EFunctionFlags::NetOwnerOnly"; }
                }
            }

            const std::string ParamsTable = Names::FunctionParamsTable(Function->Name);
            const std::string Params = Function->TopLevelParams.empty()
                ? std::string()
                : (", .Params = " + ParamsTable + ", .NumParams = (Lumina::uint16)std::size(" + ParamsTable + ")");

            Writer.Linef("{ .Name = \"%s\", .Flags = %s%s, .ReturnIndex = %d, .FrameSize = (Lumina::uint16)sizeof(%s), .Thunk = &%s },",
                Function->Name.c_str(), Flags.c_str(), Params.c_str(), Function->ReturnIndex,
                Names::FunctionParmsStruct(DisplayName, Function->Name).c_str(),
                Names::FunctionThunk(DisplayName, Function->Name).c_str());
        }
        Writer.PopIndent();
        Writer.Line("};");
        Writer.Line();
    }

    void FReflectedStruct::DefineFunctionParameterTables(FCodeWriter& Writer) const
    {
        for (const auto& Function : Functions)
        {
            if (!Function->bReflectNatively || Function->TopLevelParams.empty())
            {
                continue;
            }

            Writer.Linef("constexpr Lumina::FPropertyDesc %s::%s[] =", TypeInfoName().c_str(), Names::FunctionParamsTable(Function->Name).c_str());
            Writer.BeginBlock();
            for (const FReflectedProperty* Param : Function->TopLevelParams)
            {
                Param->EmitDescriptor(Writer);
            }
            Writer.PopIndent();
            Writer.Line("};");
            Writer.Line();
        }
    }
}
