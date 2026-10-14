using System;
using System.Runtime.InteropServices;
using LuminaSharp;

namespace Lumina;

/// Hand-written ergonomic half of SDynamicMeshComponent (the rest is Reflector-generated). Set streams then Commit; positions and indices required, normals derived if absent. Call on the game thread.
public readonly unsafe partial struct SDynamicMeshComponent
{
    /// Vertex positions (one per vertex). Required.
    public void SetPositions(ReadOnlySpan<FVector3> Positions)
        => SetPositionsData(MemoryMarshal.Cast<FVector3, float>(Positions));

    /// Vertex normals (one per vertex); optional, smooth normals are derived when omitted or mismatched.
    public void SetNormals(ReadOnlySpan<FVector3> Normals)
        => SetNormalsData(MemoryMarshal.Cast<FVector3, float>(Normals));

    /// Vertex texture coordinates (one per vertex). Optional (defaults to zero).
    public void SetUVs(ReadOnlySpan<FVector2> UVs)
        => SetUVsData(MemoryMarshal.Cast<FVector2, float>(UVs));

    /// Per-vertex colors as linear RGBA floats. Optional (defaults to white).
    public void SetColors(ReadOnlySpan<FVector4> Colors)
        => SetColorsFloatData(MemoryMarshal.Cast<FVector4, float>(Colors));

    /// Per-vertex colors as pre-packed RGBA8 (0xAABBGGRR). Optional (defaults to white).
    public void SetColors(ReadOnlySpan<uint> PackedColors)
        => SetColorsPackedData(PackedColors);

    /// Triangle indices (3 per triangle), referencing the vertex streams.
    public void SetIndices(ReadOnlySpan<int> Indices)
        => SetIndicesData(MemoryMarshal.Cast<int, uint>(Indices));

    /// Triangle indices (3 per triangle), referencing the vertex streams.
    public void SetIndices(ReadOnlySpan<uint> Indices)
        => SetIndicesData(Indices);
}
