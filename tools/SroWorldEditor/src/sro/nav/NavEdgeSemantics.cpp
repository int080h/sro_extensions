#include "sro/nav/NavEdgeSemantics.h"
#include <d3d9.h>

namespace sro::nav {

TerrainEdgePassability ClassifyTerrainEdge(uint8_t flag, int8_t d0, int8_t d1, int16_t c0, int16_t c1) {
    if (c0 < 0 || c1 < 0 || d0 < 0 || d1 < 0)
        return TerrainEdgePassability::Disconnected;
    if (EdgeFlagFullyBlocked(flag))
        return TerrainEdgePassability::Blocked;
    if (EdgeFlagOneWay(flag))
        return TerrainEdgePassability::OneWay;
    return TerrainEdgePassability::Open;
}

TerrainEdgePassability ClassifyBmsEdge(uint8_t flag, uint16_t dstCell) {
    if (dstCell != 0xFFFF && EdgeFlagBlocksCrossing(flag))
        return TerrainEdgePassability::Blocked;
    if (dstCell == 0xFFFF) {
        if (flag == 0)
            return TerrainEdgePassability::Open;
        if (EdgeFlagFullyBlocked(flag))
            return TerrainEdgePassability::Blocked;
        if (EdgeFlagOneWay(flag))
            return TerrainEdgePassability::OneWay;
    }
    if (EdgeFlagBlocksCrossing(flag))
        return TerrainEdgePassability::Blocked;
    return TerrainEdgePassability::Open;
}

const char* TerrainEdgePassabilityLabel(TerrainEdgePassability p) {
    switch (p) {
    case TerrainEdgePassability::Open: return "Open";
    case TerrainEdgePassability::OneWay: return "One-way";
    case TerrainEdgePassability::Blocked: return "Blocked";
    case TerrainEdgePassability::Disconnected: return "Disconnected";
    }
    return "?";
}

const char* EdgeFlagBitsLabel(uint8_t flag) {
    if (flag & 128) return "Siege";
    if (flag & 64) return "Bit6";
    if (flag & 32) return "Entrance";
    if (flag & 16) return "Underpass";
    if (flag & 8) return "Global";
    if (flag & 4) return "Internal";
    switch (flag & 3) {
    case 3: return "Blocked";
    case 2: return "BlockSrc2Dst";
    case 1: return "BlockDst2Src";
    default: return "None";
    }
}

unsigned int ColorForTerrainEdge(TerrainEdgePassability p, bool isGlobal) {
    switch (p) {
    case TerrainEdgePassability::Open:
        return isGlobal ? D3DCOLOR_ARGB(255, 255, 220, 60) : D3DCOLOR_ARGB(255, 60, 220, 80);
    case TerrainEdgePassability::OneWay:
        return D3DCOLOR_ARGB(255, 255, 140, 40);
    case TerrainEdgePassability::Blocked:
        return D3DCOLOR_ARGB(255, 220, 40, 40);
    case TerrainEdgePassability::Disconnected:
        return D3DCOLOR_ARGB(255, 120, 120, 120);
    }
    return D3DCOLOR_ARGB(255, 255, 255, 255);
}

unsigned int ColorForBmsOutlineEdge(uint8_t flag, bool terrainFacing, bool openToTerrain) {
    if (terrainFacing && openToTerrain)
        return D3DCOLOR_ARGB(200, 60, 200, 200);
    if (EdgeFlagBlocksCrossing(flag))
        return D3DCOLOR_ARGB(255, 255, 40, 40);
    return D3DCOLOR_ARGB(180, 255, 180, 60);
}

} // namespace sro::nav
