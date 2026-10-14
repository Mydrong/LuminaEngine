#include "Platform/Time/PlatformTime.h"
#include "RuntimePCH.h"
#include "NavMesh.h"

#include "Config/NavigationSettings.h"
#include "TaskSystem/TaskSystem.h"

// Real Recast/Detour path is gated on LUMINA_HAS_RECAST; otherwise compiles as a no-op shell.
#if defined(LUMINA_HAS_RECAST)
    #include <DetourNavMesh.h>
    #include <DetourNavMeshQuery.h>
    #include <DetourCommon.h>
#include "Log/Log.h"

// A module compiling without the define sees a narrower dtPolyRef than Detour's own sources, and the
// layout mismatch corrupts memory rather than failing to link.
static_assert(sizeof(dtPolyRef) == 8, "Recast must be built with DT_POLYREF64; see Recast.Build.cs.");
#endif

namespace Lumina
{
    namespace
    {
        FORCEINLINE void Pack(const FVector3& In, float* Out) { Out[0] = In.x; Out[1] = In.y; Out[2] = In.z; }
        FORCEINLINE FVector3 Unpack(const float* In) { return FVector3(In[0], In[1], In[2]); }

#if defined(LUMINA_HAS_RECAST)
        void ApplyFilter(const FNavQueryFilter& In, dtQueryFilter& Out)
        {
            Out.setIncludeFlags(In.IncludeFlags);
            Out.setExcludeFlags(In.ExcludeFlags);
            for (int32 i = 0; i < 64; ++i)
            {
                Out.setAreaCost(i, In.AreaCost[i]);
            }
        }

        // Mirrors the block walk in dtCreateNavMeshData and dtNavMesh::addTile.
        int32 ExpectedTileBlobSize(const dtMeshHeader* H)
        {
            auto A4 = [](int32 x) { return (x + 3) & ~3; };
#ifdef DT_POLYREF64
            auto A8 = [](int32 x) { return (x + 7) & ~7; };
            const int32 HeaderSize = A8((int32)sizeof(dtMeshHeader));
            const int32 VertsSize  = A8((int32)sizeof(float) * 3 * H->vertCount);
#else
            const int32 HeaderSize = A4((int32)sizeof(dtMeshHeader));
            const int32 VertsSize  = A4((int32)sizeof(float) * 3 * H->vertCount);
#endif
            return HeaderSize + VertsSize
                 + A4((int32)sizeof(dtPoly)               * H->polyCount)
                 + A4((int32)sizeof(dtLink)               * H->maxLinkCount)
                 + A4((int32)sizeof(dtPolyDetail)         * H->detailMeshCount)
                 + A4((int32)sizeof(float) * 3            * H->detailVertCount)
                 + A4(4                                   * H->detailTriCount)
                 + A4((int32)sizeof(dtBVNode)             * H->bvNodeCount)
                 + A4((int32)sizeof(dtOffMeshConnection)  * H->offMeshConCount);
        }

        // addTile only checks magic and version, and neither changes when the ref width or the block
        // alignment does, so a stale blob would otherwise be read at the wrong offsets.
        bool ValidateTileBlob(const TVector<uint8>& Blob, int32 TileX, int32 TileY)
        {
            if (Blob.size() < sizeof(dtMeshHeader))
            {
                LOG_ERROR("NavMesh tile ({}, {}): blob is {} bytes, smaller than a tile header. Re-bake required.",
                    TileX, TileY, (int32)Blob.size());
                return false;
            }

            const dtMeshHeader* H = reinterpret_cast<const dtMeshHeader*>(Blob.data());
            if (H->magic != DT_NAVMESH_MAGIC || H->version != DT_NAVMESH_VERSION)
            {
                LOG_ERROR("NavMesh tile ({}, {}): bad magic/version. Re-bake required.", TileX, TileY);
                return false;
            }

            const int32 Expected = ExpectedTileBlobSize(H);
            if ((int32)Blob.size() != Expected)
            {
                LOG_ERROR("NavMesh tile ({}, {}): blob is {} bytes but its header describes {}. This is navmesh data "
                          "baked against a different Detour layout (poly ref width or block alignment). Re-bake required.",
                    TileX, TileY, (int32)Blob.size(), Expected);
                return false;
            }
            return true;
        }
#endif
    }

    FNavMesh::FNavMesh() = default;

    FNavMesh::~FNavMesh()
    {
        Shutdown();
    }

    bool FNavMesh::Initialize(const FVector3& InOrigin, float InTileWorldSize, int32 MaxResidentTiles, int32 MaxPolysPerTile)
    {
        TVector<SNavTileData> None;
        return Initialize(InOrigin, InTileWorldSize, MaxResidentTiles, MaxPolysPerTile, std::move(None));
    }

    bool FNavMesh::Initialize(const FVector3& InOrigin, float InTileWorldSize, int32 MaxResidentTiles, int32 MaxPolysPerTile, TVector<SNavTileData>&& Tiles)
    {
        Shutdown();

        Origin = InOrigin;
        TileWorldSize = InTileWorldSize;

#if defined(LUMINA_HAS_RECAST)
        NavMesh = dtAllocNavMesh();
        if (!NavMesh)
        {
            LOG_ERROR("FNavMesh::Initialize: dtAllocNavMesh returned null (out of memory).");
            return false;
        }

        if (TileWorldSize <= 0.0f || MaxResidentTiles <= 0)
        {
            LOG_ERROR("FNavMesh::Initialize: invalid layout (TileWorldSize={:.3f}, MaxResidentTiles={}). Refusing to init.", TileWorldSize, MaxResidentTiles);
            dtFreeNavMesh(NavMesh);
            NavMesh = nullptr;
            return false;
        }

        dtNavMeshParams Params{};
        Pack(Origin, Params.orig);
        Params.tileWidth  = TileWorldSize;
        Params.tileHeight = TileWorldSize;
        Params.maxTiles   = MaxResidentTiles;
        Params.maxPolys   = MaxPolysPerTile;

        if (dtStatusFailed(NavMesh->init(&Params)))
        {
            LOG_ERROR("FNavMesh::Initialize: dtNavMesh::init failed (TileWorldSize={:.3f}, MaxResidentTiles={}, MaxPolys={}).", TileWorldSize, MaxResidentTiles, MaxPolysPerTile);
            dtFreeNavMesh(NavMesh);
            NavMesh = nullptr;
            return false;
        }

        // dtNavMesh::addTile is not thread-safe; the expensive bake work already happened.
        int32 Added = 0;
        int32 Skipped = 0;
        int32 Rejected = 0;
        int32 Stale = 0;
        for (SNavTileData& Tile : Tiles)
        {
            if (Tile.Blob.empty())
            {
                ++Skipped;
                continue;
            }

            if (!ValidateTileBlob(Tile.Blob, Tile.X, Tile.Y))
            {
                ++Stale;
                continue;
            }

            const size_t Size = Tile.Blob.size();
            uint8* Owned = (uint8*)dtAlloc((int)Size, DT_ALLOC_PERM);
            if (!Owned)
            {
                ++Rejected;
                continue;
            }
            memcpy(Owned, Tile.Blob.data(), Size);

            dtTileRef Ref = 0;
            const dtStatus Status = NavMesh->addTile(Owned, (int)Size, DT_TILE_FREE_DATA, 0, &Ref);
            if (dtStatusFailed(Status))
            {
                dtFree(Owned);
                ++Rejected;
            }
            else
            {
                ++Added;
            }
        }
        if (Stale > 0)
        {
            LOG_ERROR("FNavMesh::Initialize: {} of {} non-empty tiles were baked against a different Detour layout. Re-bake this navmesh.",
                Stale, Added + Rejected + Stale);
        }
        if (Rejected > 0)
        {
            LOG_WARN("FNavMesh::Initialize: dtNavMesh::addTile rejected {} of {} non-empty tiles (likely tile coord collision or maxTiles too small).",
                Rejected, Added + Rejected);
        }
        if (Added == 0 && Skipped + Rejected + Stale > 0)
        {
            LOG_ERROR("FNavMesh::Initialize: no tiles were added (skipped={}, rejected={}, stale={}). NavMesh will not be ready.", Skipped, Rejected, Stale);
            dtFreeNavMesh(NavMesh);
            NavMesh = nullptr;
            return false;
        }

        // Over-provision so contention rarely blocks; each query is a few hundred KB.
        const CNavigationSettings& Settings = *GetDefault<CNavigationSettings>();
        const uint32 PoolSize = (GTaskSystem ? GTaskSystem->GetNumWorkers() : 4u) + (uint32)Math::Max(0, Settings.QueryPoolSlack);
        QueryPool = TVector<FQuerySlot>(PoolSize);
        uint32 ReadyQueries = 0;
        for (uint32 i = 0; i < PoolSize; ++i)
        {
            dtNavMeshQuery* Query = dtAllocNavMeshQuery();
            if (Query && dtStatusSucceed(Query->init(NavMesh, Math::Max(64, Settings.QueryNodePoolSize))))
            {
                QueryPool[i].Query = Query;
                ++ReadyQueries;
            }
            else if (Query)
            {
                dtFreeNavMeshQuery(Query);
            }
        }
        if (ReadyQueries == 0)
        {
            LOG_ERROR("FNavMesh::Initialize: no dtNavMeshQuery instances initialized; pathfinding will always return false.");
            dtFreeNavMesh(NavMesh);
            NavMesh = nullptr;
            QueryPool.clear();
            return false;
        }

        bReady = true;
        bDebugCacheDirty = true;
        return true;
#else
        (void)MaxResidentTiles; (void)MaxPolysPerTile; (void)Tiles;
        LOG_ERROR("FNavMesh::Initialize: Recast/Detour not vendored (LUMINA_HAS_RECAST undefined). NavMesh cannot be initialized.");
        bReady = false;
        return false;
#endif
    }

    void FNavMesh::Shutdown()
    {
#if defined(LUMINA_HAS_RECAST)
        for (FQuerySlot& Slot : QueryPool)
        {
            if (Slot.Query) dtFreeNavMeshQuery(Slot.Query);
        }
        if (NavMesh) dtFreeNavMesh(NavMesh);
#endif
        QueryPool.clear();
        NavMesh = nullptr;
        bReady = false;
    }

    FNavMesh::FAcquiredQuery FNavMesh::AcquireQuery() const
    {
        // Linear CAS scan; spin loop is a safety net for pathological contention.
        const uint32 N = (uint32)QueryPool.size();
        if (N == 0) return {};

        for (int32 Attempt = 0; Attempt < 8; ++Attempt)
        {
            for (uint32 i = 0; i < N; ++i)
            {
                if (!QueryPool[i].Query) continue;
                bool Expected = false;
                if (QueryPool[i].Busy.compare_exchange_strong(Expected, true, std::memory_order_acq_rel))
                {
                    return FAcquiredQuery{ &QueryPool[i] };
                }
            }
            PlatformTime::YieldThread();
        }
        return {};
    }

    bool FNavMesh::ProjectPoint(const FVector3& World, const FVector3& Extents, const FNavQueryFilter& Filter, FVector3& Out) const
    {
#if defined(LUMINA_HAS_RECAST)
        FReadScopeLock Topology(TopologyLock);
        FAcquiredQuery Q = AcquireQuery();
        if (!Q) return false;

        dtQueryFilter F; ApplyFilter(Filter, F);
        float P[3]; Pack(World, P);
        float E[3]; Pack(Extents, E);
        float Nearest[3];
        dtPolyRef Ref = 0;
        if (dtStatusFailed(Q.Get()->findNearestPoly(P, E, &F, &Ref, Nearest)) || Ref == 0)
        {
            return false;
        }
        Out = Unpack(Nearest);
        return true;
#else
        (void)World; (void)Extents; (void)Filter; (void)Out;
        return false;
#endif
    }

    bool FNavMesh::FindPath(const FVector3& Start, const FVector3& End, const FNavQueryFilter& Filter, FNavPath& Out) const
    {
        Out = {};

#if defined(LUMINA_HAS_RECAST)
        FReadScopeLock Topology(TopologyLock);
        FAcquiredQuery Q = AcquireQuery();
        if (!Q)
        {
            Out.bQueryUnavailable = true;
            Out.Result = ENavPathResult::QueryUnavailable;
            return false;
        }

        Out.Epoch = TopologyEpoch.load(std::memory_order_relaxed);

        dtQueryFilter F; ApplyFilter(Filter, F);

        float SP[3]; Pack(Start, SP);
        float EP[3]; Pack(End,   EP);
        float Extents[3]; Pack(Filter.QueryExtents, Extents);

        dtPolyRef SRef = 0, ERef = 0;
        float SNear[3], ENear[3];
        if (dtStatusFailed(Q.Get()->findNearestPoly(SP, Extents, &F, &SRef, SNear)) || SRef == 0)
        {
            Out.Result = ENavPathResult::StartOffNavMesh;
            return false;
        }
        if (dtStatusFailed(Q.Get()->findNearestPoly(EP, Extents, &F, &ERef, ENear)) || ERef == 0)
        {
            Out.Result = ENavPathResult::EndOffNavMesh;
            return false;
        }

        // Both buffers live on a half-megabyte fiber stack, so the corridor can afford to be generous.
        // The settings are clamped to these ceilings rather than sized dynamically, keeping the query heap free.
        constexpr int32 MaxPolysCeiling = 1024;
        constexpr int32 MaxStraightCeiling = 512;
        const CNavigationSettings& NavSettings = *GetDefault<CNavigationSettings>();
        const int32 MaxPolys = Math::Clamp(NavSettings.MaxPathPolys, 16, MaxPolysCeiling);

        dtPolyRef Path[MaxPolysCeiling];
        int32 PathLen = 0;
        const dtStatus PathStatus = Q.Get()->findPath(SRef, ERef, SNear, ENear, &F, Path, &PathLen, MaxPolys);
        if (dtStatusFailed(PathStatus) || PathLen == 0)
        {
            Out.Result = ENavPathResult::NoRoute;
            return false;
        }

        // A corridor not ending on the goal poly stopped short, or the caller reads the last corner as the goal.
        Out.bPartial   = (PathStatus & DT_PARTIAL_RESULT) != 0 || Path[PathLen - 1] != ERef;
        Out.bTruncated = (PathStatus & (DT_BUFFER_TOO_SMALL | DT_OUT_OF_NODES)) != 0;

        // Detour gets the cap the caller can store, or the overflow comes back looking like arrival.
        const int32 RequestedCorners = Filter.MaxCorners > 0 ? Filter.MaxCorners : NavSettings.MaxPathCorners;
        const int32 MaxStraight = Math::Clamp(RequestedCorners, 8, MaxStraightCeiling);
        float StraightPath[MaxStraightCeiling * 3];
        uint8 StraightFlags[MaxStraightCeiling];
        dtPolyRef StraightRefs[MaxStraightCeiling];
        int32 StraightCount = 0;
        const dtStatus StraightStatus = Q.Get()->findStraightPath(SNear, ENear, Path, PathLen, StraightPath, StraightFlags, StraightRefs, &StraightCount, MaxStraight);
        if (dtStatusFailed(StraightStatus))
        {
            Out.Result = ENavPathResult::CornersUnavailable;
            return false;
        }
        // Detour reports a filled corner buffer as success, so the straight path silently ends mid-corridor.
        if (StraightStatus & DT_BUFFER_TOO_SMALL)
        {
            Out.bPartial   = true;
            Out.bTruncated = true;
        }

        Out.Corners.reserve(StraightCount);
        Out.CornerFlags.reserve(StraightCount);
        for (int32 i = 0; i < StraightCount; ++i)
        {
            Out.Corners.push_back(Unpack(&StraightPath[i * 3]));

            uint8 Flags = (uint8)ENavCornerFlag::None;
            if (StraightFlags[i] & DT_STRAIGHTPATH_START)                Flags |= (uint8)ENavCornerFlag::PathStart;
            if (StraightFlags[i] & DT_STRAIGHTPATH_END)                  Flags |= (uint8)ENavCornerFlag::PathEnd;
            if (StraightFlags[i] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)   Flags |= (uint8)ENavCornerFlag::OffMeshLink;
            Out.CornerFlags.push_back(Flags);
        }
        Out.bValid = true;

        // Truncated outranks partial, since a limit cut the route rather than the goal being unreachable.
        Out.Result = Out.bTruncated ? ENavPathResult::Truncated
                   : Out.bPartial   ? ENavPathResult::Partial
                                    : ENavPathResult::Success;
        return true;
#else
        (void)Start; (void)End; (void)Filter;
        Out.Result = ENavPathResult::NavigationCompiledOut;
        return false;
#endif
    }

    void FNavMesh::InvalidateDebugCache()
    {
        bDebugCacheDirty.store(true, std::memory_order_release);
    }

    void FNavMesh::EnsureDebugCache() const
    {
        if (!bDebugCacheDirty.load(std::memory_order_acquire))
        {
            return;
        }

        // Its own lock, because the rebuild writes the caches while the topology lock is only shared.
        TScopeLock<FMutex> Rebuild(DebugCacheLock);
        if (!bDebugCacheDirty.load(std::memory_order_acquire))
        {
            return;
        }

        FReadScopeLock Topology(TopologyLock);
        bDebugCacheDirty.store(false, std::memory_order_release);

        CachedTriVerts.clear();
        CachedTriAreas.clear();
        CachedBoundaryVerts.clear();
        CachedBoundaryAreas.clear();
        CachedOffMeshVerts.clear();
        CachedTileBounds.clear();

#if defined(LUMINA_HAS_RECAST)
        if (!NavMesh) return;

        const dtNavMesh* CMesh = NavMesh;
        const int32 MaxTiles = CMesh->getMaxTiles();

        size_t TriEstimate = 0;
        size_t EdgeEstimate = 0;
        size_t LinkEstimate = 0;
        for (int32 i = 0; i < MaxTiles; ++i)
        {
            const dtMeshTile* Tile = CMesh->getTile(i);
            if (!Tile || !Tile->header) continue;
            TriEstimate  += (size_t)Tile->header->detailTriCount;
            EdgeEstimate += (size_t)Tile->header->polyCount * 4;
            LinkEstimate += (size_t)Tile->header->offMeshConCount;
        }
        CachedTriVerts.reserve(TriEstimate * 3);
        CachedTriAreas.reserve(TriEstimate);
        CachedBoundaryVerts.reserve(EdgeEstimate * 2);
        CachedBoundaryAreas.reserve(EdgeEstimate);
        CachedOffMeshVerts.reserve(LinkEstimate * 2);
        CachedTileBounds.reserve(MaxTiles);

        for (int32 i = 0; i < MaxTiles; ++i)
        {
            const dtMeshTile* Tile = CMesh->getTile(i);
            if (!Tile || !Tile->header) continue;

            FNavTileBounds Tb;
            Tb.Min = FVector3(Tile->header->bmin[0], Tile->header->bmin[1], Tile->header->bmin[2]);
            Tb.Max = FVector3(Tile->header->bmax[0], Tile->header->bmax[1], Tile->header->bmax[2]);
            Tb.X   = Tile->header->x;
            Tb.Y   = Tile->header->y;
            CachedTileBounds.push_back(Tb);

            for (int p = 0; p < Tile->header->polyCount; ++p)
            {
                const dtPoly& Poly = Tile->polys[p];
                if (Poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

                const uint8 Area = Poly.getArea();

                // Outer perimeter only. neis[j]==0 -> hard boundary; nonzero (incl. DT_EXT_LINK) -> internal/cross-tile.
                for (int j = 0; j < Poly.vertCount; ++j)
                {
                    if (Poly.neis[j] != 0) continue;
                    const float* A = &Tile->verts[Poly.verts[j] * 3];
                    const float* B = &Tile->verts[Poly.verts[(j + 1) % Poly.vertCount] * 3];
                    CachedBoundaryVerts.emplace_back(A[0], A[1], A[2]);
                    CachedBoundaryVerts.emplace_back(B[0], B[1], B[2]);
                    CachedBoundaryAreas.push_back(Area);
                }

                const dtPolyDetail& Detail = Tile->detailMeshes[p];
                for (int t = 0; t < Detail.triCount; ++t)
                {
                    const uint8* Tri = &Tile->detailTris[(Detail.triBase + t) * 4];
                    for (int k = 0; k < 3; ++k)
                    {
                        const uint8 Idx = Tri[k];
                        const float* P = (Idx < Poly.vertCount)
                            ? &Tile->verts[Poly.verts[Idx] * 3]
                            : &Tile->detailVerts[(Detail.vertBase + (Idx - Poly.vertCount)) * 3];
                        CachedTriVerts.emplace_back(P[0], P[1], P[2]);
                    }
                    CachedTriAreas.push_back(Area);
                }
            }

            for (int c = 0; c < Tile->header->offMeshConCount; ++c)
            {
                const dtOffMeshConnection& Con = Tile->offMeshCons[c];
                CachedOffMeshVerts.emplace_back(Con.pos[0], Con.pos[1], Con.pos[2]);
                CachedOffMeshVerts.emplace_back(Con.pos[3], Con.pos[4], Con.pos[5]);
            }
        }
#endif
    }

    void FNavMesh::ForEachBoundaryEdge(const FBoundaryEdgeVisitor& Visitor) const
    {
        EnsureDebugCache();
        const size_t N = CachedBoundaryAreas.size();
        for (size_t i = 0; i < N; ++i)
        {
            Visitor(CachedBoundaryVerts[i * 2 + 0], CachedBoundaryVerts[i * 2 + 1], CachedBoundaryAreas[i]);
        }
    }

    void FNavMesh::ForEachOffMeshLink(const FOffMeshLinkVisitor& Visitor) const
    {
        EnsureDebugCache();
        const size_t N = CachedOffMeshVerts.size() / 2;
        for (size_t i = 0; i < N; ++i)
        {
            Visitor(CachedOffMeshVerts[i * 2 + 0], CachedOffMeshVerts[i * 2 + 1]);
        }
    }

    void FNavMesh::ForEachLoadedTile(const FTileBoundsVisitor& Visitor) const
    {
        EnsureDebugCache();
        for (const FNavTileBounds& T : CachedTileBounds)
        {
            Visitor(T);
        }
    }

    FNavDebugStats FNavMesh::GetDebugStats() const
    {
        EnsureDebugCache();
        FNavDebugStats S;
        S.LoadedTiles   = (int32)CachedTileBounds.size();
        S.Triangles     = (int32)CachedTriAreas.size();
        S.BoundaryEdges = (int32)CachedBoundaryAreas.size();
        S.OffMeshLinks  = (int32)(CachedOffMeshVerts.size() / 2);
        return S;
    }

    void FNavMesh::ForEachTriangle(FTriangleVisitor Visitor) const
    {
        EnsureDebugCache();
        const size_t NumTris = CachedTriAreas.size();
        for (size_t i = 0; i < NumTris; ++i)
        {
            Visitor(CachedTriVerts[i * 3 + 0], CachedTriVerts[i * 3 + 1], CachedTriVerts[i * 3 + 2], CachedTriAreas[i]);
        }
    }

    void FNavMesh::ParallelForEachTriangle(const FParallelTriangleVisitor& Visitor) const
    {
        EnsureDebugCache();
        const size_t NumTris = CachedTriAreas.size();
        if (NumTris == 0) return;

        Task::ParallelFor((uint32)NumTris, [this, &Visitor](uint32 i)
        {
            Visitor(CachedTriVerts[i * 3 + 0], CachedTriVerts[i * 3 + 1], CachedTriVerts[i * 3 + 2], CachedTriAreas[i]);
        });
    }

    bool FNavMesh::AddTileLocked(int32 TileX, int32 TileY, const TVector<uint8>& Blob)
    {
#if defined(LUMINA_HAS_RECAST)
        if (!NavMesh || Blob.empty() || !ValidateTileBlob(Blob, TileX, TileY))
        {
            return false;
        }

        const size_t Size = Blob.size();
        uint8* Owned = (uint8*)dtAlloc((int)Size, DT_ALLOC_PERM);
        if (!Owned)
        {
            return false;
        }
        memcpy(Owned, Blob.data(), Size);

        dtTileRef NewRef = 0;
        if (dtStatusFailed(NavMesh->addTile(Owned, (int)Size, DT_TILE_FREE_DATA, 0, &NewRef)))
        {
            dtFree(Owned);
            return false;
        }
        return true;
#else
        (void)TileX; (void)TileY; (void)Blob;
        return false;
#endif
    }

    bool FNavMesh::RemoveTileLocked(int32 TileX, int32 TileY)
    {
#if defined(LUMINA_HAS_RECAST)
        if (!NavMesh)
        {
            return false;
        }
        const dtTileRef OldRef = NavMesh->getTileRefAt(TileX, TileY, 0);
        if (OldRef != 0)
        {
            NavMesh->removeTile(OldRef, nullptr, nullptr);
        }
        return true;
#else
        (void)TileX; (void)TileY;
        return false;
#endif
    }

    void FNavMesh::RecordTileChangeLocked(int32 TileX, int32 TileY)
    {
        const uint64 Epoch = TopologyEpoch.fetch_add(1, std::memory_order_release) + 1;
        FTileChange& Slot = ChangeRing[Epoch % (uint64)ChangeRingSize];
        Slot.Epoch = Epoch;
        Slot.X = TileX;
        Slot.Y = TileY;
    }

    bool FNavMesh::HasTileChangedSince(uint64 SinceEpoch, const FVector3& Min, const FVector3& Max) const
    {
        FReadScopeLock Topology(TopologyLock);

        const uint64 Current = TopologyEpoch.load(std::memory_order_acquire);
        if (SinceEpoch >= Current)
        {
            return false;
        }
        if (Current - SinceEpoch > (uint64)ChangeRingSize || TileWorldSize <= 0.0f)
        {
            return true;
        }

        for (uint64 E = SinceEpoch + 1; E <= Current; ++E)
        {
            const FTileChange& Slot = ChangeRing[E % (uint64)ChangeRingSize];
            if (Slot.Epoch != E)
            {
                return true;
            }

            const float TileMinX = Origin.x + (float)Slot.X * TileWorldSize;
            const float TileMinZ = Origin.z + (float)Slot.Y * TileWorldSize;
            if (Max.x >= TileMinX && Min.x <= TileMinX + TileWorldSize
             && Max.z >= TileMinZ && Min.z <= TileMinZ + TileWorldSize)
            {
                return true;
            }
        }
        return false;
    }

    bool FNavMesh::RebuildTile(int32 TileX, int32 TileY, TVector<uint8>&& NewBlob)
    {
        LUMINA_PROFILE_SCOPE();

#if defined(LUMINA_HAS_RECAST)
        if (!NavMesh)
        {
            return false;
        }

        // Validated before the removal, so a rejected blob leaves the existing tile in place.
        if (!NewBlob.empty() && !ValidateTileBlob(NewBlob, TileX, TileY))
        {
            return false;
        }

        bool bAdded = true;
        {
            TScopeLock<FSharedMutex> Topology(TopologyLock);
            RemoveTileLocked(TileX, TileY);
            if (!NewBlob.empty())
            {
                bAdded = AddTileLocked(TileX, TileY, NewBlob);
            }
            RecordTileChangeLocked(TileX, TileY);
        }

        InvalidateDebugCache();
        return bAdded;
#else
        (void)TileX; (void)TileY; (void)NewBlob;
        return false;
#endif
    }

    bool FNavMesh::AddTile(int32 TileX, int32 TileY, const TVector<uint8>& Blob)
    {
        LUMINA_PROFILE_SCOPE();

        bool bResult = false;
        {
            TScopeLock<FSharedMutex> Topology(TopologyLock);
            bResult = AddTileLocked(TileX, TileY, Blob);
            if (bResult)
            {
                RecordTileChangeLocked(TileX, TileY);
            }
        }

        if (bResult)
        {
            InvalidateDebugCache();
        }
        return bResult;
    }

    bool FNavMesh::RemoveTile(int32 TileX, int32 TileY)
    {
        LUMINA_PROFILE_SCOPE();

        bool bResult = false;
        {
            TScopeLock<FSharedMutex> Topology(TopologyLock);
            bResult = RemoveTileLocked(TileX, TileY);
            RecordTileChangeLocked(TileX, TileY);
        }

        InvalidateDebugCache();
        return bResult;
    }

    bool FNavMesh::HasTile(int32 TileX, int32 TileY) const
    {
#if defined(LUMINA_HAS_RECAST)
        FReadScopeLock Topology(TopologyLock);
        return NavMesh && NavMesh->getTileRefAt(TileX, TileY, 0) != 0;
#else
        (void)TileX; (void)TileY;
        return false;
#endif
    }

    int32 FNavMesh::GetResidentTileCount() const
    {
#if defined(LUMINA_HAS_RECAST)
        FReadScopeLock Topology(TopologyLock);
        if (!NavMesh)
        {
            return 0;
        }
        int32 Count = 0;
        const dtNavMesh* CMesh = NavMesh;
        for (int32 i = 0, N = CMesh->getMaxTiles(); i < N; ++i)
        {
            const dtMeshTile* Tile = CMesh->getTile(i);
            if (Tile && Tile->header)
            {
                ++Count;
            }
        }
        return Count;
#else
        return 0;
#endif
    }

    bool FNavMesh::FindRandomPoint(const FVector3& Center, float Radius, const FNavQueryFilter& Filter, FVector3& Out) const
    {
#if defined(LUMINA_HAS_RECAST)
        FReadScopeLock Topology(TopologyLock);
        FAcquiredQuery Q = AcquireQuery();
        if (!Q) return false;

        dtQueryFilter F; ApplyFilter(Filter, F);

        float CP[3]; Pack(Center, CP);
        float Extents[3]; Pack(Filter.QueryExtents, Extents);

        dtPolyRef StartRef = 0;
        float Snapped[3];
        if (dtStatusFailed(Q.Get()->findNearestPoly(CP, Extents, &F, &StartRef, Snapped)) || StartRef == 0)
        {
            return false;
        }

        // Thread-local xorshift; cheap RNG for Detour's sampling callback.
        static thread_local uint32 RngState = 0xDEADBEEF;
        auto Rand01 = []() -> float
        {
            uint32 X = RngState ? RngState : 1u;
            X ^= X << 13; X ^= X >> 17; X ^= X << 5;
            RngState = X;
            return (X & 0x00FFFFFFu) * (1.0f / 16777216.0f);
        };

        dtPolyRef RandomRef = 0;
        float RandomPt[3];
        if (dtStatusFailed(Q.Get()->findRandomPointAroundCircle(StartRef, Snapped, Radius, &F, Rand01, &RandomRef, RandomPt)) || RandomRef == 0)
        {
            return false;
        }
        Out = Unpack(RandomPt);
        return true;
#else
        (void)Center; (void)Radius; (void)Filter; (void)Out;
        return false;
#endif
    }

    bool FNavMesh::Raycast(const FVector3& Start, const FVector3& End, const FNavQueryFilter& Filter, SNavRaycastResult& Out) const
    {
        Out = {};
        Out.Point = End;

#if defined(LUMINA_HAS_RECAST)
        FReadScopeLock Topology(TopologyLock);
        FAcquiredQuery Q = AcquireQuery();
        if (!Q) return false;

        dtQueryFilter F; ApplyFilter(Filter, F);
        float SP[3]; Pack(Start, SP);
        float EP[3]; Pack(End,   EP);
        float Extents[3]; Pack(Filter.QueryExtents, Extents);

        dtPolyRef SRef = 0;
        float SNear[3];
        if (dtStatusFailed(Q.Get()->findNearestPoly(SP, Extents, &F, &SRef, SNear)) || SRef == 0) return false;

        float T = 0.0f;
        float Normal[3];
        dtPolyRef PathRefs[64];
        int32 PathCount = 0;
        if (dtStatusFailed(Q.Get()->raycast(SRef, SNear, EP, &F, &T, Normal, PathRefs, &PathCount, 64)))
        {
            return false;
        }

        // Detour reports an unobstructed walk as T = FLT_MAX.
        Out.bHit   = T < 1.0f;
        Out.T      = Out.bHit ? T : 1.0f;
        Out.Point  = Out.bHit ? Math::Mix(Start, End, T) : End;
        Out.Normal = Out.bHit ? Unpack(Normal) : FVector3(0.0f);
        return true;
#else
        (void)Start; (void)End; (void)Filter;
        return false;
#endif
    }

    namespace NavMeshTesting
    {
        bool TileLinksAreAligned(const TVector<uint8>& Blob)
        {
#if defined(LUMINA_HAS_RECAST)
            if (Blob.size() < sizeof(dtMeshHeader))
            {
                return false;
            }
            const dtMeshHeader* H = reinterpret_cast<const dtMeshHeader*>(Blob.data());
            const int32 Expected = ExpectedTileBlobSize(H);
            if ((int32)Blob.size() != Expected)
            {
                return false;
            }

            auto A4 = [](int32 x) { return (x + 3) & ~3; };
#ifdef DT_POLYREF64
            auto A8 = [](int32 x) { return (x + 7) & ~7; };
            const int32 LinkOffset = A8((int32)sizeof(dtMeshHeader))
                                   + A8((int32)sizeof(float) * 3 * H->vertCount)
                                   + A4((int32)sizeof(dtPoly) * H->polyCount);
#else
            const int32 LinkOffset = A4((int32)sizeof(dtMeshHeader))
                                   + A4((int32)sizeof(float) * 3 * H->vertCount)
                                   + A4((int32)sizeof(dtPoly) * H->polyCount);
#endif
            return (LinkOffset % (int32)alignof(dtLink)) == 0;
#else
            (void)Blob;
            return true;
#endif
        }
    }
}
