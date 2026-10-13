#pragma once

// Leaf header on purpose: Core/Math cannot reach Core/Object, so both include this and no cycle forms.

#define NO_API

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define CONCAT4_INNER(a, b, c, d) a##b##c##d
#define CONCAT4(a, b, c, d) CONCAT4_INNER(a, b, c, d)

// The generated struct that describes a type, a friend so its tables can reach private members.
#define LUMINA_TYPE_INFO(TClass) CONCAT(FTypeInfo_, TClass)

// The specifiers these accept are listed in Applications/Reflector/Source/Reflector/ReflectionSpecifiers.h.

#if defined(REFLECTION_PARSER)

    // A type is found through its macro, since an attribute cannot reach a class from in front of the class key.
    #define GENERATED_BODY(...)
    #define REFLECT(...)

    // A declaration carries its own specifiers, so the parser reads them off the declaration it is visiting.
    #define PROPERTY(...) [[clang::annotate("lumina.property:" #__VA_ARGS__)]]
    #define FUNCTION(...) [[clang::annotate("lumina.function:" #__VA_ARGS__)]]
    #define SCRIPT_EXPORT(...) [[clang::annotate("lumina.export:" #__VA_ARGS__)]]

#else

    #define GENERATED_BODY(...) CONCAT4(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY)
    #define REFLECT(...)
    #define PROPERTY(...)
    #define FUNCTION(...)
    #define SCRIPT_EXPORT(...)

#endif
