#pragma once

#include <cfloat>
#include "AI/Navigation/NavTypes.h"
#include "Core/Threading/Sync.h"

class dtNavMesh;
class dtNavMeshQuery;

namespace Lumina
{
    struct SNavTileData;

    /** World-space AABB of a loaded nav tile + its grid coords. */
    struct FNavTileBounds
    {
        FVector3   Min = FVector3( FLT_MAX);
        FVector3   Max = FVector3(-FLT_MAX);
        int32       X = 0;
        int32       Y = 0;
    };

    /** Snapshot of cached debug data sizes. Cheap; safe to call every frame. */
    struct FNavDebugStats
    {
        int32 LoadedTiles    = 0;
        int32 Triangles      = 0;
        int32 BoundaryEdges  = 0;
        int32 OffMeshLinks   = 0;
    };

    /** Runtime navmesh: dtNavMesh + atomic-flag CAS query pool. Construct on main thread; query from any. */
    class RUNTIME_API FNavMesh
    {
    public:

        FNavMesh();
        ~FNavMesh();

        FNavMesh(const FNavMesh&) = delete;
        FNavMesh& operator=(const FNavMesh&) = delete;

        /** Empty mesh sized for MaxResidentTiles; tiles arrive through AddTile. */
        bool Initialize(const FVector3& Origin, float TileWorldSize, int32 MaxResidentTiles, int32 MaxPolysPerTile);

        /** Tiles is consumed; each blob is copied into Detour memory. Every non-empty tile is made resident,
         *  so MaxResidentTiles must be at least that many. */
        bool Initialize(const FVector3& Origin, float TileWorldSize, int32 MaxResidentTiles, int32 MaxPolysPerTile, TVector<SNavTileData>&& Tiles);

        void Shutdown();

        bool IsReady() const { return bReady; }

        bool ProjectPoint(const FVector3& World, const FVector3& Extents, const FNavQueryFilter& Filter, FVector3& Out) const;
        bool FindPath(const FVector3& Start, const FVector3& End, const FNavQueryFilter& Filter, FNavPath& Out) const;

        /** Walks the surface from Start toward End. False when the query could not run; Out.bHit says whether a wall stopped it. */
        bool Raycast(const FVector3& Start, const FVector3& End, const FNavQueryFilter& Filter, SNavRaycastResult& Out) const;

        bool FindRandomPoint(const FVector3& Center, float Radius, const FNavQueryFilter& Filter, FVector3& Out) const;

        /** NewBlob is consumed; empty removes the tile. Serializes against in-flight queries itself. */
        bool RebuildTile(int32 TileX, int32 TileY, TVector<uint8>&& NewBlob);

        /** Pages a baked tile in. Fails if the resident pool is full or the blob does not match this layout. */
        bool AddTile(int32 TileX, int32 TileY, const TVector<uint8>& Blob);

        /** Pages a tile out. True when the slot is free afterwards, including when it already was. */
        bool RemoveTile(int32 TileX, int32 TileY);

        bool HasTile(int32 TileX, int32 TileY) const;

        int32 GetResidentTileCount() const;

        /** Bumped whenever a tile is added or removed. An FNavPath carries the value it was found
         *  against, so a follower can tell the ground moved under it. */
        uint64 GetTopologyEpoch() const { return TopologyEpoch.load(std::memory_order_acquire); }

        // Y is ignored because a tile spans the whole bake, and a ring overflow reports changed.
        bool HasTileChangedSince(uint64 SinceEpoch, const FVector3& Min, const FVector3& Max) const;

        /** Iterates the cached flat triangle list (skip the dtNavMesh traversal cost). */
        using FTriangleVisitor = TMoveOnlyFunction<void(const FVector3&, const FVector3&, const FVector3&, uint8)>;
        void ForEachTriangle(FTriangleVisitor Visitor) const;

        /** Visitor MUST be thread-safe; invoked concurrently. */
        using FParallelTriangleVisitor = TFunction<void(const FVector3&, const FVector3&, const FVector3&, uint8)>;
        void ParallelForEachTriangle(const FParallelTriangleVisitor& Visitor) const;

        /** Outer perimeter of each poly (neis[edge]==0). Excludes internal poly-to-poly edges. */
        using FBoundaryEdgeVisitor = TFunction<void(const FVector3& A, const FVector3& B, uint8 Area)>;
        void ForEachBoundaryEdge(const FBoundaryEdgeVisitor& Visitor) const;

        /** Off-mesh connection start/end pairs (jump links, teleporters, etc.). */
        using FOffMeshLinkVisitor = TFunction<void(const FVector3& Start, const FVector3& End)>;
        void ForEachOffMeshLink(const FOffMeshLinkVisitor& Visitor) const;

        /** Loaded tile world-AABBs from dtMeshTile::header. */
        using FTileBoundsVisitor = TFunction<void(const FNavTileBounds&)>;
        void ForEachLoadedTile(const FTileBoundsVisitor& Visitor) const;

        FNavDebugStats GetDebugStats() const;

        /** Marks the debug draw caches stale. Rebuilding them walks every poly, so it waits for a reader. */
        void InvalidateDebugCache();

    private:

        struct FQuerySlot
        {
            dtNavMeshQuery*     Query = nullptr;
            std::atomic<bool>   Busy{ false };

            FQuerySlot() = default;

            // Relocating a pool slot has to carry the flag by hand, because an atomic is not movable.
            FQuerySlot(FQuerySlot&& Other) noexcept
                : Query(Other.Query)
                , Busy(Other.Busy.load(std::memory_order_relaxed))
            {
                Other.Query = nullptr;
            }

            FQuerySlot& operator=(FQuerySlot&& Other) noexcept
            {
                Query = Other.Query;
                Busy.store(Other.Busy.load(std::memory_order_relaxed), std::memory_order_relaxed);
                Other.Query = nullptr;
                return *this;
            }
        };

        struct FAcquiredQuery
        {
            FQuerySlot* Slot = nullptr;
            ~FAcquiredQuery() { if (Slot) Slot->Busy.store(false, std::memory_order_release); }
            dtNavMeshQuery* Get() const { return Slot ? Slot->Query : nullptr; }
            explicit operator bool() const { return Slot != nullptr; }
        };

        FAcquiredQuery AcquireQuery() const;

        /** All three assume TopologyLock is already held exclusively. */
        bool AddTileLocked(int32 TileX, int32 TileY, const TVector<uint8>& Blob);
        bool RemoveTileLocked(int32 TileX, int32 TileY);
        void RecordTileChangeLocked(int32 TileX, int32 TileY);

        /** Rebuilds the debug caches if stale. Main thread only, like every reader of them. */
        void EnsureDebugCache() const;

    private:

        dtNavMesh*                          NavMesh = nullptr;

        // Shared by every query, exclusive for tile mutation. addTile rewrites the link arrays of up to
        // eight neighbours, so a tile swap is never local and the whole mesh is the only safe granularity.
        mutable FSharedMutex                TopologyLock;

        std::atomic<uint64>                 TopologyEpoch{ 1 };

        // Keyed by epoch so a follower can ask whether a change touched its own corridor.
        struct FTileChange
        {
            uint64 Epoch = 0;
            int32  X = 0;
            int32  Y = 0;
        };
        static constexpr int32              ChangeRingSize = 256;
        FTileChange                         ChangeRing[ChangeRingSize] = {};

        // Mutable so const query API can flip Busy flags.
        mutable TVector<FQuerySlot>         QueryPool;

        // Debug draw only, so all of it is built on demand and mutable behind the const readers.
        mutable FMutex                      DebugCacheLock;
        mutable std::atomic<bool>           bDebugCacheDirty{ true };

        // Flat cache: 3 vec3 per tri in Verts; 1 area byte per tri.
        mutable TVector<FVector3>          CachedTriVerts;
        mutable TVector<uint8>              CachedTriAreas;

        // 2 vec3 per edge in Verts; 1 area byte per edge. Boundary = poly outer perimeter.
        mutable TVector<FVector3>          CachedBoundaryVerts;
        mutable TVector<uint8>              CachedBoundaryAreas;

        // 2 vec3 per link (Start, End).
        mutable TVector<FVector3>          CachedOffMeshVerts;

        mutable TVector<FNavTileBounds>     CachedTileBounds;

        FVector3                           Origin = FVector3(0.0f);
        float                               TileWorldSize = 0.0f;
        bool                                bReady = false;
    };

    /** Probes into the baked tile layout, so tests can assert it without including Detour. */
    namespace NavMeshTesting
    {
        RUNTIME_API bool TileLinksAreAligned(const TVector<uint8>& Blob);
    }
}
