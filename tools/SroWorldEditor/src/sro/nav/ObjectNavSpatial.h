#pragma once
#include <vector>
#include <cstdint>
#include "PlacementVM.h"
#include "parsers/BmsParser.h"
#include "formats/NavMeshFormat.h"
#include "core/Math.h"

namespace sro::nav {

Vector3 TransformPlacementNavVertex(const Vector3& v, const PlacementVM& p, int centerRx, int centerRy);

// EdgeFlag: BlockDst2Src=1, BlockSrc2Dst=2, Blocked=3
inline bool IsBmsEdgeFullyBlocked(uint8_t edgeFlag) {
    return (edgeFlag & 3) == 3;
}

inline bool IsBmsTerrainFacingEdge(const BmsNavEdge& e) {
    return e.DstCell == 0xFFFF;
}

inline bool IsBmsEdgeOpenToTerrain(const BmsNavEdge& e) {
    return IsBmsTerrainFacingEdge(e) && e.Flag == 0;
}

enum class BmsObjectNavKind {
    NoNav = 0,
    Building,
    WalkablePlatform
};

enum class BmsCellNavKind {
    Building,
    Platform,
    NoNav
};

BmsObjectNavKind ClassifyPlacementNav(const BmsNavMesh& nav);
BmsCellNavKind ClassifyBmsCell(const BmsNavMesh& nav, int cellIndex);
const char* BmsObjectNavKindLabel(BmsObjectNavKind kind);
const char* BmsCellNavKindLabel(BmsCellNavKind kind);

struct BmsEdgeStats {
    int outlineTotal = 0;
    int outlineOpenTerrain = 0;
    int outlineBlocked = 0;
    int outlineTerrainFacing = 0;
    int inlineTotal = 0;
};

BmsEdgeStats CountBmsEdges(const BmsNavMesh& nav);

enum class PassabilityKind : uint8_t {
    Passable = 0,
    TerrainBlocked,
    TerrainEdgeBlocked,
    ObjectBuilding,
    ObjectPlatform,
    Oob
};

struct TileBmsStats {
    int buildingCells = 0;
    int platformCells = 0;
    int placementsOnTile = 0;
};

struct TileNavDiagnostics {
    PassabilityKind kind = PassabilityKind::Passable;
    int32_t cellId = -1;
    uint16_t flags = 0;
    uint16_t textureId = 0;
    int planeIndex = -1;
    TileBmsStats bms;
    std::vector<int> quadCellObjIndices;
    int crossingInternalEdges = 0;
    int crossingBlockedEdges = 0;
};

const char* PassabilityKindLabel(PassabilityKind kind);
const char* PassabilityVerdict(PassabilityKind kind);

void BuildObjectBuildingFootprint(const std::vector<PlacementVM>& placements,
                                  int centerRx, int centerRy,
                                  std::vector<uint8_t>& outMask);

void BuildObjectPlatformFootprint(const std::vector<PlacementVM>& placements,
                                  int centerRx, int centerRy,
                                  std::vector<uint8_t>& outMask);

void BuildObjectNavCoverageFootprint(const std::vector<PlacementVM>& placements,
                                     int centerRx, int centerRy,
                                     std::vector<uint8_t>& outMask);

void BuildCombinedPassabilityMask(const sro::formats::NavMesh& nav,
                                  const std::vector<PlacementVM>& placements,
                                  int centerRx, int centerRy,
                                  std::vector<PassabilityKind>& outKinds);

void QueryTileNavDiagnostics(const sro::formats::NavMesh& nav,
                             const std::vector<PlacementVM>& placements,
                             int centerRx, int centerRy, int tx, int tz,
                             TileNavDiagnostics& out);

void QueryTileBmsStats(const std::vector<PlacementVM>& placements,
                       int centerRx, int centerRy, int tx, int tz,
                       TileBmsStats& outStats);

bool ComputePlacementFootprintBounds(const PlacementVM& p, int centerRx, int centerRy,
                                     int& minTx, int& minTz, int& maxTx, int& maxTz);

int CountRegionPlacementsWithBms(const std::vector<PlacementVM>& placements, int rx, int ry);

// Placement diagnostics for Object Nav Inspector.
struct PlacementNavDiagnostics {
    BmsObjectNavKind kind = BmsObjectNavKind::NoNav;
    BmsEdgeStats edges;
    int nvmObjectIndex = -1;
    std::vector<int> cellQuadIndices;
    int terrainFacingOpen = 0;
    int terrainFacingBlocked = 0;
    uint32_t navFlag = 0;
    int eventCount = 0;
    bool inNvm = false;
    bool hasBms = false;
};

PlacementNavDiagnostics BuildPlacementDiagnostics(const sro::formats::NavMesh* nvm,
                                                  const PlacementVM& p,
                                                  int centerRx, int centerRy);

} // namespace sro::nav
