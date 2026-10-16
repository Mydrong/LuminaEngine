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

    // A live view of the row named RowName, throwing when it is absent or T is not the table's row type exactly.
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

    // Exact match, mirroring the C++ FindDataTableRow bound, so a near-miss type cannot reinterpret fields.
    private bool RowTypeIs<T>() where T : struct, INativeStruct<T>
    {
        return string.Equals(RowStructName, T.NativeName, StringComparison.Ordinal);
    }
}

/// Handwritten extensions to the reflected <see cref="SDataTableRowHandle"/> wrapper.
public readonly unsafe partial struct SDataTableRowHandle
{
    /// True when nothing is referenced at all, as opposed to referencing a row that fails to resolve.
    public bool IsNull => DataTable == null || string.IsNullOrEmpty(RowName);

    public T Get<T>() where T : struct, INativeStruct<T>
        => TryGet<T>() ?? throw new InvalidOperationException($"Row handle does not resolve to a {T.NativeName}.");

    // Null when the handle is empty, the name is not in the table, or the table's rows are not T.
    public T? TryGet<T>() where T : struct, INativeStruct<T>
    {
        IntPtr Row = GetRowMemory();
        if (Row == IntPtr.Zero || DataTable is not { } Table
            || !string.Equals(Table.RowStructName, T.NativeName, StringComparison.Ordinal))
        {
            return null;
        }
        return NativeStructs.FromAddress<T>(Row);
    }
}
