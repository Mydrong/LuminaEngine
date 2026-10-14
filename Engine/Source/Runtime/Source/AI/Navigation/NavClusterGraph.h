#pragma once

#include <atomic>
#include "Containers/Vector.h"
#include "Core/Math/Math.h"
#include "Core/Threading/Sync.h"

class dtNavMesh;
class dtQueryFilter;
struct dtMeshTile;

namespace Lumina
{
    // The polys of each tile grouped into clusters connected inside it, so a long path plans tile by tile before Detour walks polys.
    class FNavClusterGraph
    {
    public:

        // One cluster on a route, entered through EntryRef, with Distance measured along the route from the start.
        struct FStep
        {
            uint64   Key = 0;
            uint64   EntryRef = 0;
            FVector3 Center = FVector3(0.0f);
            float    Distance = 0.0f;
        };

        enum class ERoute : uint8
        {
            Failed,
            Reached,
            Unreachable,
        };

        void Reset(int32 MaxTiles);

        // Every resident tile at once, for a mesh that was just filled.
        void Rebuild(const dtNavMesh& Mesh);

        // Called after Detour has linked or unlinked the tile, so the neighbors' edges are read as they now stand.
        void AddTile(const dtNavMesh& Mesh, const dtMeshTile& Tile);
        void RemoveTile(const dtNavMesh& Mesh, uint32 TileIndex, int32 TileX, int32 TileY);

        // Unreachable routes to the cluster nearest the goal that the start can reach.
        ERoute FindRoute(const dtNavMesh& Mesh, const dtQueryFilter& Filter, uint64 StartRef, uint64 EndRef,
                         const FVector3& StartPos, const FVector3& EndPos, TVector<FStep>& Out) const;

        // The poly of a route's cluster whose center lies nearest Pos.
        uint64 NearestPoly(const dtNavMesh& Mesh, uint64 Key, const FVector3& Pos) const;

    private:

        struct FEdge
        {
            uint64 Key = 0;
            uint64 EntryRef = 0;
            uint16 Flags = 0;
            uint8  Area = 0;
        };

        struct FTileClusters
        {
            uint32            Salt = 0;
            TVector<uint16>   PolyCluster;
            TVector<FVector3> Centers;
            TVector<FEdge>    Edges;
            TVector<uint32>   EdgeStart;
        };

        const FTileClusters* FindTile(const dtNavMesh& Mesh, uint64 PolyRef) const;
        uint64 KeyOf(const dtNavMesh& Mesh, uint64 PolyRef) const;
        void BuildClusters(const dtNavMesh& Mesh, const dtMeshTile& Tile);
        void BuildEdges(const dtNavMesh& Mesh, const dtMeshTile& Tile);
        void BuildNeighborEdges(const dtNavMesh& Mesh, int32 TileX, int32 TileY);
        void EnsureComponents() const;
        uint32 ComponentOf(uint64 Key) const;
        ERoute Search(const dtNavMesh& Mesh, const dtQueryFilter& Filter, uint64 StartKey, uint64 StartRef, uint64 GoalKey,
                      const FVector3& StartPos, const FVector3& GoalPos, TVector<FStep>& Out) const;

        TVector<FTileClusters> Tiles;
        uint64 Version = 1;

        // Labeled on the first long query after a change, which already holds the topology lock shared.
        mutable FMutex                 ComponentLock;
        mutable std::atomic<uint64>    ComponentVersion{ 0 };

        // Undirected labels by cluster, so different labels prove no route while equal ones still need a search.
        mutable TVector<uint32>        ClusterOffset;
        mutable TVector<uint32>        Components;
    };
}
