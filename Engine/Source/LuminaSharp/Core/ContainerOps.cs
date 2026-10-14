namespace LuminaSharp;

// Prefixes of the native FVectorOps and FMapOps, kept at the same offsets since native only appends to them.

#pragma warning disable CS0649 // fields are populated by overlaying native memory, not managed assignment

// Mirror of native Lumina::FVectorOps, C# calling only PushBack, RemoveAt and Clear.
internal unsafe struct VectorOps
{
    public delegate* unmanaged[Cdecl]<void*, nuint> Size;
    public delegate* unmanaged[Cdecl]<void*, void*> Data;
    public delegate* unmanaged[Cdecl]<void*, void*, void> PushBack;
    public delegate* unmanaged[Cdecl]<void*, nuint, void> RemoveAt;
    public delegate* unmanaged[Cdecl]<void*, void> Clear;
    public delegate* unmanaged[Cdecl]<void*, nuint, void> Resize;
    public delegate* unmanaged[Cdecl]<void*, nuint, void> Reserve;
    public delegate* unmanaged[Cdecl]<void*, nuint, nuint, void> Swap;
    public uint ElementSize;
}

// Mirror of native Lumina::FMapOps, where a C++ bool return is one byte.
internal unsafe struct MapOps
{
    public delegate* unmanaged[Cdecl]<void*, nuint> Size;
    public delegate* unmanaged[Cdecl]<void*, void*, void*, void*> Insert;
    public delegate* unmanaged[Cdecl]<void*, void*, void*> Find;
    public delegate* unmanaged[Cdecl]<void*, void*, byte> RemoveByKey;
    public delegate* unmanaged[Cdecl]<void*, void> Clear;
    public delegate* unmanaged[Cdecl]<void*, nuint, void> Reserve;
    public delegate* unmanaged[Cdecl]<void*, void*, void*, void> ForEach;
    public delegate* unmanaged[Cdecl]<void*, void*, void> ConstructKey;
    public delegate* unmanaged[Cdecl]<void*, void*, void> DestructKey;
    public delegate* unmanaged[Cdecl]<void*, nuint, void**, void**, void> At;
    public uint KeySize;
    public uint ValueSize;
}

#pragma warning restore CS0649
