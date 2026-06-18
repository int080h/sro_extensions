#pragma once
#include <cstdint>

namespace sro::nav {

enum class TerrainEdgePassability {
    Open,
    OneWay,
    Blocked,
    Disconnected
};

// Shared EdgeFlag semantics (NVM internal/global + BMS PrimMeshNavEdgeFlag).
inline bool EdgeFlagBlocksCrossing(uint8_t flag) { return (flag & 3) != 0; }
inline bool EdgeFlagFullyBlocked(uint8_t flag) { return (flag & 3) == 3; }
inline bool EdgeFlagOneWay(uint8_t flag) {
    const uint8_t b = flag & 3;
    return b == 1 || b == 2;
}

TerrainEdgePassability ClassifyTerrainEdge(uint8_t flag, int8_t d0, int8_t d1, int16_t c0, int16_t c1);
TerrainEdgePassability ClassifyBmsEdge(uint8_t flag, uint16_t dstCell);

const char* TerrainEdgePassabilityLabel(TerrainEdgePassability p);
const char* EdgeFlagBitsLabel(uint8_t flag);

// D3DCOLOR_ARGB helpers for terrain edge rendering.
unsigned int ColorForTerrainEdge(TerrainEdgePassability p, bool isGlobal);
unsigned int ColorForBmsOutlineEdge(uint8_t flag, bool terrainFacing, bool openToTerrain);

} // namespace sro::nav
