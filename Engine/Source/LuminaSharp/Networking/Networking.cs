using System;
using JetBrains.Annotations;
using System.Collections.Generic;

namespace LuminaSharp;

// The host is always connection 0, which is also what a standalone world answers as.
public readonly struct Connection : IEquatable<Connection>
{
    public readonly uint Id;

    public Connection(uint Id)
    {
        this.Id = Id;
    }

    public static Connection Host => default;

    public static Connection Local => new(Networking.LocalConnectionId(Engine.World));

    public bool IsHost => Id == 0;

    public bool IsLocal => Id == Networking.LocalConnectionId(Engine.World);

    public bool Equals(Connection Other) => Id == Other.Id;
    public override bool Equals(object? Obj) => Obj is Connection Other && Equals(Other);
    public override int GetHashCode() => (int)Id;
    public static bool operator ==(Connection A, Connection B) => A.Id == B.Id;
    public static bool operator !=(Connection A, Connection B) => A.Id != B.Id;
    public override string ToString() => IsHost ? "Host" : $"Connection {Id}";
}

// The session of the current world. A standalone world counts as its own host.
public static class Networking
{
    public static bool IsActive => Engine.World.IsNetworked;

    public static bool IsHost => !Engine.World.IsClient;

    public static bool IsClient => Engine.World.IsClient;

    public static bool IsDedicatedServer => Engine.World.GetNetMode() == Lumina.ENetMode.DedicatedServer;

    // A load-test client from -bots, which gameplay should drive itself and never draw for.
    public static bool IsBot => Engine.World.IsBotWorld();

    // Remote connections that finished joining, which a client has no view of.
    public static IReadOnlyList<Connection> Connections => GetConnections(Engine.World);

    // A host and a standalone world are always in session, and a client is once it has joined.
    public static bool IsConnected => !Engine.World.IsClient || Engine.World.IsNetJoined();

    // The entity Connection owns, preferring one with a character controller, or FEntity.Null.
    public static FEntity PawnOf(Connection Connection) => Engine.World.GetOwnedPawn(Connection.Id);

    // This peer's own pawn, found by ownership.
    public static FEntity LocalPawn => PawnOf(Connection.Local);

    internal static uint LocalConnectionId(Lumina.CWorld World)
    {
        return World.IsClient ? World.GetLocalNetConnection() : 0u;
    }

    internal static IReadOnlyList<Connection> GetConnections(Lumina.CWorld World)
    {
        if (!World.IsServer)
        {
            return Array.Empty<Connection>();
        }

        uint[] Ids = World.GetNetConnections();
        var Result = new Connection[Ids.Length];
        for (int Index = 0; Index < Ids.Length; ++Index)
        {
            Result[Index] = new Connection(Ids[Index]);
        }
        return Result;
    }
}

// Who controls an entity, and whether this peer does.
public readonly struct EntityNetwork
{
    private readonly Lumina.CWorld World;
    private readonly FEntity Entity;

    public EntityNetwork(Lumina.CWorld World, FEntity Entity)
    {
        this.World = World;
        this.Entity = Entity;
    }

    public static EntityNetwork Of(FEntity Entity) => new(Engine.World, Entity);

    // Needs an SNetworkComponent on the entity.
    public bool IsNetworked => World.IsNetworked && World.IsEntityNetworked(Entity);

    public Connection Owner => new(World.GetNetOwner(Entity));

    // Always true in a standalone world, where nothing else could control it.
    public bool IsOwner => !IsNetworked || Owner.Id == Networking.LocalConnectionId(World);

    // Another peer controls the entity, so this one only mirrors it.
    public bool IsProxy => IsNetworked && !IsOwner;

    // Host only, a client asking is refused.
    public bool AssignOwnership(Connection Connection) => World.SetNetOwner(Entity, Connection.Id);

    public bool DropOwnership() => AssignOwnership(Connection.Host);

    public uint NetId => World.EntityToNetId(Entity);

    // Resends replicated component fields. A [Property(Sync)] field marks itself when written.
    public void MarkDirty() => World.MarkEntityNetDirty(Entity);
}

// Mark a void method on an CEntityScript with one of these and calling it routes the call.
public static class Rpc
{
    public abstract class RpcAttribute : Attribute
    {
        protected RpcAttribute(Lumina.ENetFlags Flags)
        {
            this.Flags = Flags;
        }

        public Lumina.ENetFlags Flags { get; }
    }

    // Runs on the host and every client.
    [MeansImplicitUse]
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public sealed class BroadcastAttribute : RpcAttribute
    {
        public BroadcastAttribute(Lumina.ENetFlags Flags = Lumina.ENetFlags.None) : base(Flags) { }
    }

    [MeansImplicitUse]
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public sealed class HostAttribute : RpcAttribute
    {
        public HostAttribute(Lumina.ENetFlags Flags = Lumina.ENetFlags.None) : base(Flags) { }
    }

    // Runs on whichever peer owns the entity.
    [MeansImplicitUse]
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    public sealed class OwnerAttribute : RpcAttribute
    {
        public OwnerAttribute(Lumina.ENetFlags Flags = Lumina.ENetFlags.None) : base(Flags) { }
    }

    // The local connection when the call did not arrive from the network.
    public static Connection Caller => RpcRuntime.CurrentCaller();

    public static bool IsRemote => RpcRuntime.IsReceiving;
}
