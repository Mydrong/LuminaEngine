using System;
using System.Runtime.InteropServices;

namespace LuminaSharp;

// Entities carrying any of these are left out of a view, mirroring ECS::TExclude.
public readonly struct Exclude
{
    internal readonly IntPtr[]? Tokens;

    private Exclude(IntPtr[] Tokens)
    {
        this.Tokens = Tokens;
    }

    public static Exclude None => default;

    public static Exclude Of<T1>() where T1 : struct, IComponent<T1> => new(ExcludeSet<T1>.Tokens);

    public static Exclude Of<T1, T2>() where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2>
        => new(ExcludeSet<T1, T2>.Tokens);

    public static Exclude Of<T1, T2, T3>() where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3>
        => new(ExcludeSet<T1, T2, T3>.Tokens);

    public static Exclude Of<T1, T2, T3, T4>() where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4>
        => new(ExcludeSet<T1, T2, T3, T4>.Tokens);

    private static class ExcludeSet<T1> where T1 : struct, IComponent<T1>
    {
        public static readonly IntPtr[] Tokens = { ComponentOps<T1>.Token };
    }

    private static class ExcludeSet<T1, T2> where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2>
    {
        public static readonly IntPtr[] Tokens = { ComponentOps<T1>.Token, ComponentOps<T2>.Token };
    }

    private static class ExcludeSet<T1, T2, T3> where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3>
    {
        public static readonly IntPtr[] Tokens = { ComponentOps<T1>.Token, ComponentOps<T2>.Token, ComponentOps<T3>.Token };
    }

    private static class ExcludeSet<T1, T2, T3, T4> where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4>
    {
        public static readonly IntPtr[] Tokens = { ComponentOps<T1>.Token, ComponentOps<T2>.Token, ComponentOps<T3>.Token, ComponentOps<T4>.Token };
    }
}

// Walks a native view one chunk per crossing, handing out tracked handles so one kept past the loop stays safe.
internal unsafe struct ViewCursor : IDisposable
{
    private const int ChunkSize = 1024;
    private const int MaxIncludes = 4;

    private readonly ulong World;
    private readonly int Includes;
    private fixed long Tokens[MaxIncludes];
    private fixed long EpochSlots[MaxIncludes];
    private uint* Entities;
    private IntPtr* Pointers;
    private IntPtr State;
    private int Count;
    private int Index;

    public ViewCursor(ulong World, ReadOnlySpan<IntPtr> IncludeTokens, Exclude Filter)
    {
        this.World = World;
        Includes = IncludeTokens.Length;
        Entities = (uint*)NativeMemory.Alloc(ChunkSize * sizeof(uint));
        Pointers = (IntPtr*)NativeMemory.Alloc((nuint)(ChunkSize * Includes) * (nuint)sizeof(IntPtr));
        Count = 0;
        Index = -1;

        IntPtr* Include = stackalloc IntPtr[MaxIncludes];
        for (int i = 0; i < Includes; ++i)
        {
            Tokens[i] = IncludeTokens[i];
            Include[i] = IncludeTokens[i];
        }

        IntPtr[]? Excluded = Filter.Tokens;
        fixed (IntPtr* Exclude = Excluded)
        {
            State = Native.ViewBegin(World, Include, Includes, Exclude, Excluded?.Length ?? 0);
        }

        uint** Slots = stackalloc uint*[MaxIncludes];
        if (State != IntPtr.Zero)
        {
            Native.ViewIncludeEpochs(State, Slots, Includes);
        }
        for (int i = 0; i < Includes; ++i)
        {
            EpochSlots[i] = State != IntPtr.Zero ? (long)Slots[i] : 0;
        }
    }

    public bool MoveNext()
    {
        if (State == IntPtr.Zero)
        {
            return false;
        }
        if (++Index < Count)
        {
            return true;
        }
        Count = Native.ViewNextChunk(State, Entities, Pointers, ChunkSize, Includes);
        Index = 0;
        return Count > 0;
    }

    public FEntity Entity => new(Entities[Index]);

    public T Get<T>(int Include) where T : struct, INativeStruct
    {
        uint EntityId = Entities[Index];
        IntPtr Pointer = Pointers[Index * Includes + Include];
        return NativeStructs.FromRef<T>(new NativeRef(Pointer, World, EntityId, (IntPtr)Tokens[Include], (uint*)EpochSlots[Include]));
    }

    public void Dispose()
    {
        if (State != IntPtr.Zero)
        {
            Native.ViewEnd(State);
            State = IntPtr.Zero;
        }
        if (Entities != null)
        {
            NativeMemory.Free(Entities);
            NativeMemory.Free(Pointers);
            Entities = null;
            Pointers = null;
        }
    }
}

public readonly struct View<T1> where T1 : struct, IComponent<T1>
{
    private readonly ulong World;
    private readonly Exclude Filter;

    internal View(ulong World, Exclude Filter)
    {
        this.World = World;
        this.Filter = Filter;
    }

    private ViewCursor Open() => new(World, stackalloc IntPtr[] { ComponentOps<T1>.Token }, Filter);

    public void Each(Action<FEntity, T1> Body)
    {
        using ViewCursor Cursor = Open();
        while (Cursor.MoveNext())
        {
            Body(Cursor.Entity, Cursor.Get<T1>(0));
        }
    }

    public Enumerator GetEnumerator() => new(Open());

    public struct Enumerator : IDisposable
    {
        private ViewCursor Cursor;

        internal Enumerator(ViewCursor Cursor) => this.Cursor = Cursor;

        public (FEntity Entity, T1 First) Current => (Cursor.Entity, Cursor.Get<T1>(0));

        public bool MoveNext() => Cursor.MoveNext();

        public void Dispose() => Cursor.Dispose();
    }
}

public readonly struct View<T1, T2> where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2>
{
    private readonly ulong World;
    private readonly Exclude Filter;

    internal View(ulong World, Exclude Filter)
    {
        this.World = World;
        this.Filter = Filter;
    }

    private ViewCursor Open() => new(World, stackalloc IntPtr[] { ComponentOps<T1>.Token, ComponentOps<T2>.Token }, Filter);

    public void Each(Action<FEntity, T1, T2> Body)
    {
        using ViewCursor Cursor = Open();
        while (Cursor.MoveNext())
        {
            Body(Cursor.Entity, Cursor.Get<T1>(0), Cursor.Get<T2>(1));
        }
    }

    public Enumerator GetEnumerator() => new(Open());

    public struct Enumerator : IDisposable
    {
        private ViewCursor Cursor;

        internal Enumerator(ViewCursor Cursor) => this.Cursor = Cursor;

        public (FEntity Entity, T1 First, T2 Second) Current => (Cursor.Entity, Cursor.Get<T1>(0), Cursor.Get<T2>(1));

        public bool MoveNext() => Cursor.MoveNext();

        public void Dispose() => Cursor.Dispose();
    }
}

public readonly struct View<T1, T2, T3> where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3>
{
    private readonly ulong World;
    private readonly Exclude Filter;

    internal View(ulong World, Exclude Filter)
    {
        this.World = World;
        this.Filter = Filter;
    }

    private ViewCursor Open() => new(World, stackalloc IntPtr[] { ComponentOps<T1>.Token, ComponentOps<T2>.Token, ComponentOps<T3>.Token }, Filter);

    public void Each(Action<FEntity, T1, T2, T3> Body)
    {
        using ViewCursor Cursor = Open();
        while (Cursor.MoveNext())
        {
            Body(Cursor.Entity, Cursor.Get<T1>(0), Cursor.Get<T2>(1), Cursor.Get<T3>(2));
        }
    }

    public Enumerator GetEnumerator() => new(Open());

    public struct Enumerator : IDisposable
    {
        private ViewCursor Cursor;

        internal Enumerator(ViewCursor Cursor) => this.Cursor = Cursor;

        public (FEntity Entity, T1 First, T2 Second, T3 Third) Current => (Cursor.Entity, Cursor.Get<T1>(0), Cursor.Get<T2>(1), Cursor.Get<T3>(2));

        public bool MoveNext() => Cursor.MoveNext();

        public void Dispose() => Cursor.Dispose();
    }
}

public readonly struct View<T1, T2, T3, T4> where T1 : struct, IComponent<T1> where T2 : struct, IComponent<T2> where T3 : struct, IComponent<T3> where T4 : struct, IComponent<T4>
{
    private readonly ulong World;
    private readonly Exclude Filter;

    internal View(ulong World, Exclude Filter)
    {
        this.World = World;
        this.Filter = Filter;
    }

    private ViewCursor Open() => new(World, stackalloc IntPtr[] { ComponentOps<T1>.Token, ComponentOps<T2>.Token, ComponentOps<T3>.Token, ComponentOps<T4>.Token }, Filter);

    public void Each(Action<FEntity, T1, T2, T3, T4> Body)
    {
        using ViewCursor Cursor = Open();
        while (Cursor.MoveNext())
        {
            Body(Cursor.Entity, Cursor.Get<T1>(0), Cursor.Get<T2>(1), Cursor.Get<T3>(2), Cursor.Get<T4>(3));
        }
    }

    public Enumerator GetEnumerator() => new(Open());

    public struct Enumerator : IDisposable
    {
        private ViewCursor Cursor;

        internal Enumerator(ViewCursor Cursor) => this.Cursor = Cursor;

        public (FEntity Entity, T1 First, T2 Second, T3 Third, T4 Fourth) Current => (Cursor.Entity, Cursor.Get<T1>(0), Cursor.Get<T2>(1), Cursor.Get<T3>(2), Cursor.Get<T4>(3));

        public bool MoveNext() => Cursor.MoveNext();

        public void Dispose() => Cursor.Dispose();
    }
}
