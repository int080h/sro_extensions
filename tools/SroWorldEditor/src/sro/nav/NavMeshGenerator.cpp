#include "nav/NavMeshGenerator.h"
#include "nav/TileCoord.h"
#include <vector>

namespace sro::nav {

namespace {

bool IsWalkable(const formats::NavMesh& nav, int tx, int ty) {
    if (!IsValidTile(tx, ty)) return false;
    const auto& tile = nav.TileMap[static_cast<size_t>(TileIndex(tx, ty))];
    if (tile.Flags == 0xFFFF) return false;
    return (tile.Flags & 0x0001) == 0;
}

void RegenerateInternalEdges(formats::NavMesh& nav) {
    nav.InternalEdges.clear();
    const int cellCount = static_cast<int>(nav.Cells.size());
    if (cellCount == 0) return;

    auto cellAtTile = [&](int tx, int ty) -> int {
        if (!IsValidTile(tx, ty)) return -1;
        const int32_t id = nav.TileMap[static_cast<size_t>(TileIndex(tx, ty))].CellID;
        if (id < 0 || id >= cellCount) return -1;
        return static_cast<int>(id);
    };

    for (int ty = 0; ty < kTilesPerSide; ++ty) {
        for (int tx = 0; tx < kTilesPerSide; ++tx) {
            const int c0 = cellAtTile(tx, ty);
            if (c0 < 0) continue;

            if (tx + 1 < kTilesPerSide) {
                const int c1 = cellAtTile(tx + 1, ty);
                if (c1 >= 0 && c1 != c0) {
                    float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
                    TileBoundsLocal(tx + 1, ty, x0, z0, x1, z1);
                    formats::NavInternalEdge e;
                    e.MinX = x0; e.MinY = z0; e.MaxX = x0; e.MaxY = z1;
                    e.Flags = 0;
                    e.C0 = static_cast<int16_t>(c0);
                    e.C1 = static_cast<int16_t>(c1);
                    e.D0 = 1;
                    e.D1 = 3;
                    nav.InternalEdges.push_back(e);
                }
            }
            if (ty + 1 < kTilesPerSide) {
                const int c1 = cellAtTile(tx, ty + 1);
                if (c1 >= 0 && c1 != c0) {
                    float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
                    TileBoundsLocal(tx, ty + 1, x0, z0, x1, z1);
                    formats::NavInternalEdge e;
                    e.MinX = x0; e.MinY = z0; e.MaxX = x1; e.MaxY = z0;
                    e.Flags = 0;
                    e.C0 = static_cast<int16_t>(c0);
                    e.C1 = static_cast<int16_t>(c1);
                    e.D0 = 2;
                    e.D1 = 0;
                    nav.InternalEdges.push_back(e);
                }
            }
        }
    }
    nav.InternalEdgeCount = static_cast<uint32_t>(nav.InternalEdges.size());
}

uint32_t CountOpenCells(const formats::NavMesh& nav) {
    uint32_t open = 0;
    for (const auto& tile : nav.TileMap) {
        if (tile.Flags == 0xFFFF) continue;
        if ((tile.Flags & 0x0001) == 0) ++open;
    }
    return open;
}

} // namespace

void NavMeshGenerator::RegenerateCells(formats::NavMesh& nav) {
    std::vector<uint8_t> visited(kTilesPerSide * kTilesPerSide, 0);
    nav.Cells.clear();

    for (int ty = 0; ty < kTilesPerSide; ++ty) {
        for (int tx = 0; tx < kTilesPerSide; ++tx) {
            const int idx = TileIndex(tx, ty);
            if (visited[static_cast<size_t>(idx)] || !IsWalkable(nav, tx, ty)) continue;

            int w = 1, h = 1;
            while (tx + w < kTilesPerSide) {
                bool ok = true;
                for (int y = ty; y < ty + h && ok; ++y)
                    if (!IsWalkable(nav, tx + w, y)) ok = false;
                if (!ok) break;
                ++w;
            }
            while (ty + h < kTilesPerSide) {
                bool ok = true;
                for (int x = tx; x < tx + w && ok; ++x)
                    if (!IsWalkable(nav, x, ty + h)) ok = false;
                if (!ok) break;
                ++h;
            }

            for (int y = ty; y < ty + h; ++y)
                for (int x = tx; x < tx + w; ++x)
                    visited[static_cast<size_t>(TileIndex(x, y))] = 1;

            formats::NavCell cell;
            cell.MinX = static_cast<float>(tx) * kTileSize;
            cell.MinY = static_cast<float>(ty) * kTileSize;
            cell.MaxX = static_cast<float>(tx + w) * kTileSize;
            cell.MaxY = static_cast<float>(ty + h) * kTileSize;
            nav.Cells.push_back(cell);
        }
    }

    nav.TotalCellCount = static_cast<uint32_t>(nav.Cells.size());

    for (int ty = 0; ty < kTilesPerSide; ++ty) {
        for (int tx = 0; tx < kTilesPerSide; ++tx) {
            nav.TileMap[static_cast<size_t>(TileIndex(tx, ty))].CellID = -1;
        }
    }

    for (uint32_t ci = 0; ci < nav.Cells.size(); ++ci) {
        const auto& c = nav.Cells[ci];
        const int tminX = static_cast<int>(c.MinX / kTileSize);
        const int tminY = static_cast<int>(c.MinY / kTileSize);
        if (tminX >= 0 && tminY >= 0 && tminX < kTilesPerSide && tminY < kTilesPerSide)
            nav.TileMap[static_cast<size_t>(TileIndex(tminX, tminY))].CellID = static_cast<int32_t>(ci);
    }

    RegenerateInternalEdges(nav);
    nav.OpenCellCount = CountOpenCells(nav);
}

} // namespace sro::nav
