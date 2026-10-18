using System;
using System.Collections.Generic;
using LuminaSharp;

namespace Lumina;

/// Handwritten extensions to the reflected <see cref="CDataTable"/> wrapper, adding typed row lookup.
public unsafe partial class CDataTable
{
    /// Every row name in authored order.
    public List<string> RowNames
    {
        get
        {
            int Count = GetRowCount();
            var Names = new List<string>(Count);
            for (int Index = 0; Index < Count; ++Index)
            {
                Names.Add(GetRowNameAt(Index));
            }
            return Names;
        }
    }

    // A live view of the row named RowName, throwing when it is absent or the rows are neither T nor derived from it.
    public T GetRow<T>(string RowName) where T : struct, INativeStruct<T>
        => TryGetRow<T>(RowName) ?? throw new InvalidOperationException($"DataTable has no {T.NativeName} row named '{RowName}'.");

    public T? TryGetRow<T>(string RowName) where T : struct, INativeStruct<T>
    {
        return RowTypeIs<T>() ? NativeStructs.FromAddressOrNull<T>(FindRow(RowName)) : null;
    }

    public T GetRowAt<T>(int Index) where T : struct, INativeStruct<T>
        => TryGetRowAt<T>(Index) ?? throw new InvalidOperationException($"DataTable has no {T.NativeName} row at {Index}.");

    public T? TryGetRowAt<T>(int Index) where T : struct, INativeStruct<T>
    {
        return RowTypeIs<T>() ? NativeStructs.FromAddressOrNull<T>(GetRowMemoryAt(Index)) : null;
    }

    // A derived row starts with its base, so it reads as any struct it derives from and as nothing else.
    internal bool RowTypeIs<T>() where T : struct, INativeStruct<T>
    {
        return string.Equals(RowStructName, T.NativeName, StringComparison.Ordinal) || RowsAreNamed(T.NativeName);
    }
}

/// Handwritten extensions to the reflected <see cref="SDataTableRowHandle"/> wrapper.
public readonly unsafe partial struct SDataTableRowHandle
{
    /// True when nothing is referenced at all, as opposed to referencing a row that fails to resolve.
    public bool IsNull => DataTable == null || string.IsNullOrEmpty(RowName);

    public T Get<T>() where T : struct, INativeStruct<T>
        => TryGet<T>() ?? throw new InvalidOperationException($"Row handle does not resolve to a {T.NativeName}.");

    // Null when the handle is empty, the name is not in the table, or the rows are neither T nor derived from it.
    public T? TryGet<T>() where T : struct, INativeStruct<T>
    {
        IntPtr Row = GetRowMemory();
        if (Row == IntPtr.Zero || DataTable is not { } Table || !Table.RowTypeIs<T>())
        {
            return null;
        }
        return NativeStructs.FromAddress<T>(Row);
    }
}
