#pragma once

namespace Lumina::Reflection
{
    class FReflectionDatabase;
    class FReflectedEnum;
    class FReflectedStruct;
    class FReflectedClass;
    class FReflectedHeader;
    class FCodeWriter;

    /**
     * Emits the C# mirror of a reflected type into a .generated.cs file: standalone C# that compiles
     * into the engine's managed API assembly so user scripts get a typed, code-completed API.
     *
     * Mapping:
     *   - enum            -> C# enum
     *   - blittable struct -> [StructLayout(Sequential)] value struct (all fields are blittable)
     *   - other struct    -> handle struct over native memory (INativeStruct)
     *   - class (CObject) -> opaque handle wrapper (NativeObject)
     *
     * Each Emit* returns true if it wrote anything. Opt-out via the `NoCSharp` metadata.
     */
    namespace CSharpBindingEmitter
    {
        bool EmitForEnum(FCodeWriter& Writer, const FReflectedEnum& Enum);
        bool EmitForStruct(FCodeWriter& Writer, const FReflectedStruct& Struct, const FReflectionDatabase& Database);
        bool EmitForClass(FCodeWriter& Writer, const FReflectedClass& Class, const FReflectionDatabase& Database);

        // Emits a native static_assert validating that a blittable struct's C++ size matches the C#
        // mirror's computed size (catches non-reflected fields that would corrupt the mirror). Writes
        // nothing for non-blittable structs. Goes into the .generated.cpp (the type must be defined).
        void EmitNativeLayoutCheck(FCodeWriter& Writer, const FReflectedStruct& Struct, const FReflectionDatabase& Database);

        // Emits the `extern "C" RUNTIME_API` property get/set thunks backing the C# wrappers' scalar
        // properties (read/write a public numeric/bool member by value). Into the .generated.cpp.
        // Accepts struct OR class (pass the FReflectedStruct base).
        void EmitNativeThunks(FCodeWriter& Writer, const FReflectedStruct& Type, const FReflectionDatabase& Database);

        // SCRIPT_EXPORT free functions declared in a header: the C# static partial-class [NativeCall] bindings
        // (into .generated.cs) and the native extern "C" thunks (into .generated.cpp). No-op if the header
        // declares none. Module + API macro come from the header's project, like reflected-type methods.
        bool EmitFreeFunctions(FCodeWriter& Writer, FReflectedHeader* Header, const FReflectionDatabase& Database);
        void EmitNativeFreeFunctions(FCodeWriter& Writer, FReflectedHeader* Header, const FReflectionDatabase& Database);
    }
}
