#include "sro/nav/ObjectNavSpatial.h"
#include "sro/nav/TileCoord.h"
#include "sro/nav/NavEdgeSemantics.h"
#include "core/SceneSpace.h"

namespace sro::nav {

namespace {
bool IsTerrainOob(uint16_t flags) { return flags == 0xFFFF; }
bool IsTerrainBlocked(uint16_t flags) { return !IsTerrainOob(flags) && (flags & 0x0001) != 0; }

void MarkFootprintTile(std::vector<uint8_t>& mask, float sceneX, float sceneZ,
                       int centerRx, int centerRy) {
    int tx = 0, tz = 0;
    if (!SceneToTile(sceneX, sceneZ, centerRx, centerRy, tx, tz))
        return;
    mask[static_cast<size_t>(TileIndex(tx, tz))] = 1;
}

void AccumulateCellsForPlacement(const PlacementVM& p, int centerRx, int centerRy,
                                 std::vector<uint8_t>& outMask,
                                 const BmsNavMesh& nav,
                                 BmsCellNavKind kindFilter) {
    for (int ci = 0; ci < static_cast<int>(nav.Cells.size()); ++ci) {
        if (ClassifyBmsCell(nav, ci) != kindFilter)
            continue;
        const auto& cell = nav.Cells[static_cast<size_t>(ci)];
        auto vtx = [&](uint16_t vi) -> Vector3 {
            const Vector3 local = (vi < nav.Vertices.size()) ? nav.Vertices[vi].Position : Vector3{};
            return TransformPlacementNavVertex(local, p, centerRx, centerRy);
        };
        const Vector3 a = vtx(cell.V0);
        const Vector3 b = vtx(cell.V1);
        const Vector3 c = vtx(cell.V2);
        MarkFootprintTile(outMask, a.x, a.z, centerRx, centerRy);
        MarkFootprintTile(outMask, b.x, b.z, centerRx, centerRy);
        MarkFootprintTile(outMask, c.x, c.z, centerRx, centerRy);
        const Vector3 cen((a.x + b.x + c.x) / 3.0f, 0.0f, (a.z + b.z + c.z) / 3.0f);
        MarkFootprintTile(outMask, cen.x, cen.z, centerRx, centerRy);
    }
}

void AccumulateFootprintByCellKind(const std::vector<PlacementVM>& placements,
                                   int centerRx, int centerRy,
                                   BmsCellNavKind kindFilter,
                                   std::vector<uint8_t>& outMask) {
    outMask.assign(kTilesPerSide * kTilesPerSide, 0);
    for (const auto& p : placements) {
        if (p.LoadedRx != centerRx || p.LoadedRy != centerRy)
            continue;
        const BmsNavMesh* nav = p.Collision.NavMesh;
        if (!nav || nav->Cells.empty())
            continue;
        AccumulateCellsForPlacement(p, centerRx, centerRy, outMask, *nav, kindFilter);
    }
}

bool CellTouchesOpenTerrainEdge(const BmsNavMesh& nav, int cellIndex) {
    for (const auto& e : nav.OutlineEdges) {
        if (!IsBmsEdgeOpenToTerrain(e))
            continue;
        if (e.SrcCell == static_cast<uint16_t>(cellIndex) || e.DstCell == static_cast<uint16_t>(cellIndex))
            return true;
    }
    return false;
}
} // namespace

Vector3 TransformPlacementNavVertex(const Vector3& v, const PlacementVM& p, int centerRx, int centerRy) {
    int objRx = (p.Object.RegionID >> 8) & 0xFF;
    int objRy = p.Object.RegionID & 0xFF;
    if (objRx == 0 && objRy == 0) {
        objRx = p.LoadedRx;
        objRy = p.LoadedRy;
    }
    const float oX = RegionOffsetX(objRy, centerRy);
    const float oZ = RegionOffsetZ(objRx, centerRx);
    const float c = cosf(p.Object.Yaw);
    const float s = sinf(p.Object.Yaw);
    const float lx = v.x * c - v.z * s;
    const float lz = v.x * s + v.z * c;
    return Vector3(lx + p.Object.PosX + oX, v.y + p.Object.PosY, lz + p.Object.PosZ + oZ);
}

BmsCellNavKind ClassifyBmsCell(const BmsNavMesh& nav, int cellIndex) {
    if (cellIndex < 0 || cellIndex >= static_cast<int>(nav.Cells.size()))
        return BmsCellNavKind::NoNav;
    if (CellTouchesOpenTerrainEdge(nav, cellIndex))
        return BmsCellNavKind::Platform;
    return BmsCellNavKind::Building;
}

BmsObjectNavKind ClassifyPlacementNav(const BmsNavMesh& nav) {
    if (nav.Cells.empty())
        return BmsObjectNavKind::NoNav;
    bool anyPlatform = false;
    for (int i = 0; i < static_cast<int>(nav.Cells.size()); ++i) {
        if (ClassifyBmsCell(nav, i) == BmsCellNavKind::Platform)
            anyPlatform = true;
    }
    return anyPlatform ? BmsObjectNavKind::WalkablePlatform : BmsObjectNavKind::Building;
}

const char* BmsCellNavKindLabel(BmsCellNavKind kind) {
    switch (kind) {
    case BmsCellNavKind::Building: return "Building cell";
    case BmsCellNavKind::Platform: return "Platform cell";
    case BmsCellNavKind::NoNav: return "No nav";
    }
    return "?";
}

const char* BmsObjectNavKindLabel(BmsObjectNavKind kind) {
    switch (kind) {
    case BmsObjectNavKind::NoNav: return "No nav mesh";
    case BmsObjectNavKind::Building: return "Building (not walkable)";
    case BmsObjectNavKind::WalkablePlatform: return "Walkable platform";
    }
    return "Unknown";
}

BmsEdgeStats CountBmsEdges(const BmsNavMesh& nav) {
    BmsEdgeStats stats;
    stats.inlineTotal = static_cast<int>(nav.InlineEdges.size());
    for (const auto& e : nav.OutlineEdges) {
        ++stats.outlineTotal;
        if (IsBmsTerrainFacingEdge(e)) {
            ++stats.outlineTerrainFacing;
            if (IsBmsEdgeOpenToTerrain(e))
                ++stats.outlineOpenTerrain;
            else
                ++stats.outlineBlocked;
        } else if (IsBmsEdgeFullyBlocked(e.Flag) || EdgeFlagBlocksCrossing(e.Flag)) {
            ++stats.outlineBlocked;
        }
    }
    return stats;
}

const char* PassabilityKindLabel(PassabilityKind kind) {
    switch (kind) {
    case PassabilityKind::Passable: return "Terrain corridor";
    case PassabilityKind::TerrainBlocked: return "Terrain blocked";
    case PassabilityKind::TerrainEdgeBlocked: return "Terrain edge blocked";
    case PassabilityKind::ObjectBuilding: return "Building BMS";
    case PassabilityKind::ObjectPlatform: return "Walkable platform";
    case PassabilityKind::Oob: return "OOB";
    }
    return "Unknown";
}

const char* PassabilityVerdict(PassabilityKind kind) {
    switch (kind) {
    case PassabilityKind::Passable: return "PASSABLE — terrain corridor";
    case PassabilityKind::TerrainBlocked: return "NOT PASSABLE — terrain tile blocked (bit 0)";
    case PassabilityKind::TerrainEdgeBlocked: return "NOT PASSABLE — terrain edge blocks crossing";
    case PassabilityKind::ObjectBuilding: return "NOT PASSABLE — building BMS volume";
    case PassabilityKind::ObjectPlatform: return "PASSABLE — walkable platform BMS surface";
    case PassabilityKind::Oob: return "NOT PASSABLE — out of bounds";
    }
    return "Unknown";
}

void BuildObjectBuildingFootprint(const std::vector<PlacementVM>& placements,
                                  int centerRx, int centerRy,
                                  std::vector<uint8_t>& outMask) {
    AccumulateFootprintByCellKind(placements, centerRx, centerRy, BmsCellNavKind::Building, outMask);
}

void BuildObjectPlatformFootprint(const std::vector<PlacementVM>& placements,
                                  int centerRx, int centerRy,
                                  std::vector<uint8_t>& outMask) {
    AccumulateFootprintByCellKind(placements, centerRx, centerRy, BmsCellNavKind::Platform, outMask);
}

void BuildObjectNavCoverageFootprint(const std::vector<PlacementVM>& placements,
                                     int centerRx, int centerRy,
                                     std::vector<uint8_t>& outMask) {
    outMask.assign(kTilesPerSide * kTilesPerSide, 0);
    for (const auto& p : placements) {
        if (p.LoadedRx != centerRx || p.LoadedRy != centerRy)
            continue;
        if (!p.Collision.NavMesh || p.Collision.NavMesh->Cells.empty())
            continue;
        for (const auto& cell : p.Collision.NavMesh->Cells) {
            auto vtx = [&](uint16_t vi) -> Vector3 {
                const Vector3 local = (vi < p.Collision.NavMesh->Vertices.size())
                    ? p.Collision.NavMesh->Vertices[vi].Position : Vector3{};
                return TransformPlacementNavVertex(local, p, centerRx, centerRy);
            };
            const Vector3 a = vtx(cell.V0);
            const Vector3 b = vtx(cell.V1);
            const Vector3 c = vtx(cell.V2);
            MarkFootprintTile(outMask, a.x, a.z, centerRx, centerRy);
            MarkFootprintTile(outMask, b.x, b.z, centerRx, centerRy);
            MarkFootprintTile(outMask, c.x, c.z, centerRx, centerRy);
            const Vector3 cen((a.x + b.x + c.x) / 3.0f, 0.0f, (a.z + b.z + c.z) / 3.0f);
            MarkFootprintTile(outMask, cen.x, cen.z, centerRx, centerRy);
        }
    }
}

static bool TileCrossesEdgeLine(float ex0, float ey0, float ex1, float ey1, int tx, int tz) {
    float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    TileBoundsLocal(tx, tz, x0, z0, x1, z1);
    auto segIntersect = [&](float ax, float ay, float bx, float by,
                            float cx, float cy, float dx, float dy) -> bool {
        auto orient = [](float px, float py, float qx, float qy, float rx, float ry) {
            return (qy - py) * (rx - qx) - (qx - px) * (ry - qy);
        };
        const float o1 = orient(ax, ay, bx, by, cx, cy);
        const float o2 = orient(ax, ay, bx, by, dx, dy);
        const float o3 = orient(cx, cy, dx, dy, ax, ay);
        const float o4 = orient(cx, cy, dx, dy, bx, by);
        if (o1 * o2 < 0 && o3 * o4 < 0) return true;
        return false;
    };
    const float corners[4][2] = {{x0, z0}, {x1, z0}, {x1, z1}, {x0, z1}};
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        if (segIntersect(corners[i][0], corners[i][1], corners[j][0], corners[j][1], ex0, ey0, ex1, ey1))
            return true;
    }
    return (ex0 >= x0 && ex0 <= x1 && ey0 >= z0 && ey0 <= z1);
}

static bool TileCrossesEdge(const sro::formats::NavInternalEdge& e, int tx, int tz) {
    return TileCrossesEdgeLine(e.MinX, e.MinY, e.MaxX, e.MaxY, tx, tz);
}

static bool TileCrossesGlobalEdge(const sro::formats::NavGlobalEdge& e, int tx, int tz) {
    return TileCrossesEdgeLine(e.MinX, e.MinY, e.MaxX, e.MaxY, tx, tz);
}

static bool TileHasBlockedTerrainEdge(const sro::formats::NavMesh& nav, int tx, int tz) {
    for (const auto& e : nav.InternalEdges) {
        if (!TileCrossesEdge(e, tx, tz))
            continue;
        if (ClassifyTerrainEdge(e.Flags, e.D0, e.D1, e.C0, e.C1) != TerrainEdgePassability::Open)
            return true;
    }
    for (const auto& e : nav.GlobalEdges) {
        if (!TileCrossesGlobalEdge(e, tx, tz))
            continue;
        if (ClassifyTerrainEdge(e.Flags, e.D0, e.D1, e.C0, e.C1) != TerrainEdgePassability::Open)
            return true;
    }
    return false;
}

void BuildCombinedPassabilityMask(const sro::formats::NavMesh& nav,
                                  const std::vector<PlacementVM>& placements,
                                  int centerRx, int centerRy,
                                  std::vector<PassabilityKind>& outKinds) {
    outKinds.assign(kTilesPerSide * kTilesPerSide, PassabilityKind::Passable);

    std::vector<uint8_t> buildingFp;
    std::vector<uint8_t> platformFp;
    BuildObjectBuildingFootprint(placements, centerRx, centerRy, buildingFp);
    BuildObjectPlatformFootprint(placements, centerRx, centerRy, platformFp);

    const int tileCount = std::min<int>(kTilesPerSide * kTilesPerSide, static_cast<int>(nav.TileMap.size()));
    for (int idx = 0; idx < tileCount; ++idx) {
        const int tx = idx % kTilesPerSide;
        const int tz = idx / kTilesPerSide;
        const auto& tile = nav.TileMap[static_cast<size_t>(idx)];
        if (IsTerrainOob(tile.Flags)) {
            outKinds[static_cast<size_t>(idx)] = PassabilityKind::Oob;
        } else if (IsTerrainBlocked(tile.Flags)) {
            outKinds[static_cast<size_t>(idx)] = PassabilityKind::TerrainBlocked;
        } else if (TileHasBlockedTerrainEdge(nav, tx, tz)) {
            outKinds[static_cast<size_t>(idx)] = PassabilityKind::TerrainEdgeBlocked;
        } else if (buildingFp[static_cast<size_t>(idx)]) {
            outKinds[static_cast<size_t>(idx)] = PassabilityKind::ObjectBuilding;
        } else if (platformFp[static_cast<size_t>(idx)]) {
            outKinds[static_cast<size_t>(idx)] = PassabilityKind::ObjectPlatform;
        } else {
            outKinds[static_cast<size_t>(idx)] = PassabilityKind::Passable;
        }
    }
}

void QueryTileBmsStats(const std::vector<PlacementVM>& placements,
                       int centerRx, int centerRy, int tx, int tz,
                       TileBmsStats& outStats) {
    outStats = {};
    for (const auto& p : placements) {
        if (p.LoadedRx != centerRx || p.LoadedRy != centerRy)
            continue;
        const BmsNavMesh* nav = p.Collision.NavMesh;
        if (!nav)
            continue;

        bool touches = false;
        for (int ci = 0; ci < static_cast<int>(nav->Cells.size()); ++ci) {
            const auto& cell = nav->Cells[static_cast<size_t>(ci)];
            auto vtx = [&](uint16_t vi) -> Vector3 {
                const Vector3 local = (vi < nav->Vertices.size()) ? nav->Vertices[vi].Position : Vector3{};
                return TransformPlacementNavVertex(local, p, centerRx, centerRy);
            };
            const Vector3 cen((vtx(cell.V0).x + vtx(cell.V1).x + vtx(cell.V2).x) / 3.0f, 0.0f,
                              (vtx(cell.V0).z + vtx(cell.V1).z + vtx(cell.V2).z) / 3.0f);
            int cellTx = 0, cellTz = 0;
            if (!SceneToTile(cen.x, cen.z, centerRx, centerRy, cellTx, cellTz))
                continue;
            if (cellTx != tx || cellTz != tz)
                continue;
            touches = true;
            if (ClassifyBmsCell(*nav, ci) == BmsCellNavKind::Building)
                ++outStats.buildingCells;
            else
                ++outStats.platformCells;
        }
        if (touches)
            ++outStats.placementsOnTile;
    }
}

void QueryTileNavDiagnostics(const sro::formats::NavMesh& nav,
                             const std::vector<PlacementVM>& placements,
                             int centerRx, int centerRy, int tx, int tz,
                             TileNavDiagnostics& out) {
    out = {};
    if (!IsValidTile(tx, tz))
        return;
    const int idx = TileIndex(tx, tz);
    if (idx < static_cast<int>(nav.TileMap.size())) {
        const auto& tile = nav.TileMap[static_cast<size_t>(idx)];
        out.cellId = tile.CellID;
        out.flags = tile.Flags;
        out.textureId = tile.TextureID;
        out.planeIndex = PlaneIndexFromTile(tx, tz);
    }
    {
        std::vector<PassabilityKind> kinds;
        BuildCombinedPassabilityMask(nav, placements, centerRx, centerRy, kinds);
        out.kind = kinds[static_cast<size_t>(idx)];
    }
    QueryTileBmsStats(placements, centerRx, centerRy, tx, tz, out.bms);
    if (out.cellId >= 0 && out.cellId < static_cast<int32_t>(nav.Cells.size())) {
        out.quadCellObjIndices.assign(nav.Cells[static_cast<size_t>(out.cellId)].ObjIndices.begin(),
                                      nav.Cells[static_cast<size_t>(out.cellId)].ObjIndices.end());
    }
    for (const auto& e : nav.InternalEdges) {
        if (TileCrossesEdge(e, tx, tz)) {
            ++out.crossingInternalEdges;
            if (ClassifyTerrainEdge(e.Flags, e.D0, e.D1, e.C0, e.C1) != TerrainEdgePassability::Open)
                ++out.crossingBlockedEdges;
        }
    }
}

bool ComputePlacementFootprintBounds(const PlacementVM& p, int centerRx, int centerRy,
                                     int& minTx, int& minTz, int& maxTx, int& maxTz) {
    const BmsNavMesh* nav = p.Collision.NavMesh;
    if (!nav || nav->Cells.empty())
        return false;
    minTx = minTz = kTilesPerSide;
    maxTx = maxTz = -1;
    bool any = false;
    for (const auto& cell : nav->Cells) {
        auto vtx = [&](uint16_t vi) -> Vector3 {
            const Vector3 local = (vi < nav->Vertices.size()) ? nav->Vertices[vi].Position : Vector3{};
            return TransformPlacementNavVertex(local, p, centerRx, centerRy);
        };
        const Vector3 pts[4] = {
            vtx(cell.V0), vtx(cell.V1), vtx(cell.V2),
            Vector3((vtx(cell.V0).x + vtx(cell.V1).x + vtx(cell.V2).x) / 3.0f, 0.0f,
                    (vtx(cell.V0).z + vtx(cell.V1).z + vtx(cell.V2).z) / 3.0f)
        };
        for (const auto& pt : pts) {
            int tx = 0, tz = 0;
            if (!SceneToTile(pt.x, pt.z, centerRx, centerRy, tx, tz))
                continue;
            any = true;
            minTx = std::min(minTx, tx);
            minTz = std::min(minTz, tz);
            maxTx = std::max(maxTx, tx);
            maxTz = std::max(maxTz, tz);
        }
    }
    return any;
}

int CountRegionPlacementsWithBms(const std::vector<PlacementVM>& placements, int rx, int ry) {
    int count = 0;
    for (const auto& p : placements) {
        if (p.LoadedRx == rx && p.LoadedRy == ry && p.Collision.NavMesh)
            ++count;
    }
    return count;
}

PlacementNavDiagnostics BuildPlacementDiagnostics(const sro::formats::NavMesh* nvm,
                                                  const PlacementVM& p,
                                                  int centerRx, int centerRy) {
    PlacementNavDiagnostics d;
    d.hasBms = p.Collision.NavMesh != nullptr;
    if (p.Collision.NavMesh) {
        d.kind = ClassifyPlacementNav(*p.Collision.NavMesh);
        d.edges = CountBmsEdges(*p.Collision.NavMesh);
        d.navFlag = p.Collision.NavMesh->NavFlag;
        d.eventCount = static_cast<int>(p.Collision.NavMesh->Events.size());
        for (const auto& e : p.Collision.NavMesh->OutlineEdges) {
            if (!IsBmsTerrainFacingEdge(e))
                continue;
            if (IsBmsEdgeOpenToTerrain(e))
                ++d.terrainFacingOpen;
            else
                ++d.terrainFacingBlocked;
        }
    }
    if (nvm) {
        for (size_t i = 0; i < nvm->Objects.size(); ++i) {
            if (nvm->Objects[i].LocalUID == p.Object.UID) {
                d.inNvm = true;
                d.nvmObjectIndex = static_cast<int>(i);
                for (size_t ci = 0; ci < nvm->Cells.size(); ++ci) {
                    for (uint16_t oi : nvm->Cells[ci].ObjIndices) {
                        if (oi == static_cast<uint16_t>(i))
                            d.cellQuadIndices.push_back(static_cast<int>(ci));
                    }
                }
                break;
            }
        }
    }
    (void)centerRx;
    (void)centerRy;
    return d;
}

} // namespace sro::nav
