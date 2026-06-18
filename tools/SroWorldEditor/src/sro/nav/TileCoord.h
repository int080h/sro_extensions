#pragma once
#include "formats/NavMeshFormat.h"
#include "core/SceneSpace.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace sro::nav {

inline constexpr int kTilesPerSide = sro::formats::NavMesh::kTileExtent;
inline constexpr float kTileSize = sro::formats::NavMesh::kTileSize;
inline constexpr float kRegionSize = kTilesPerSide * kTileSize;
inline constexpr int kHeightSamples = sro::formats::NavMesh::kHeightExtent;
inline constexpr int kPlanesPerSide = sro::formats::NavMesh::kPlaneExtent;
inline constexpr float kPlaneSize = sro::formats::NavMesh::kPlaneSize;
inline constexpr int kTilesPerPlane = static_cast<int>(kPlaneSize / kTileSize);

inline int TileIndex(int tx, int tz) { return tz * kTilesPerSide + tx; }

inline bool IsValidTile(int tx, int tz) {
    return tx >= 0 && tz >= 0 && tx < kTilesPerSide && tz < kTilesPerSide;
}

inline bool SceneToTile(float sceneX, float sceneZ, int centerRx, int centerRy, int& outTx, int& outTz) {
    const sro::LocalPos lp = sro::SceneSpace::SceneToLocal(sceneX, sceneZ, centerRx, centerRy);
    if (lp.rx != centerRx || lp.ry != centerRy)
        return false;
    outTx = static_cast<int>(lp.localX / kTileSize);
    outTz = static_cast<int>(lp.localZ / kTileSize);
    return IsValidTile(outTx, outTz);
}

inline void TileCenterLocal(int tx, int tz, float& outX, float& outZ) {
    outX = (static_cast<float>(tx) + 0.5f) * kTileSize;
    outZ = (static_cast<float>(tz) + 0.5f) * kTileSize;
}

inline void TileBoundsLocal(int tx, int tz, float& x0, float& z0, float& x1, float& z1) {
    x0 = static_cast<float>(tx) * kTileSize;
    z0 = static_cast<float>(tz) * kTileSize;
    x1 = x0 + kTileSize;
    z1 = z0 + kTileSize;
}

inline int PlaneIndexFromTile(int tx, int tz) {
    const int px = tx / kTilesPerPlane;
    const int pz = tz / kTilesPerPlane;
    if (px < 0 || pz < 0 || px >= kPlanesPerSide || pz >= kPlanesPerSide)
        return -1;
    return pz * kPlanesPerSide + px;
}

inline int HeightVertexIndex(int vx, int vz) { return vz * kHeightSamples + vx; }

inline float SampleHeightBilinear(const std::vector<float>& hm, float localX, float localZ) {
    if (hm.empty()) return 0.0f;
    const float fx = localX / kTileSize;
    const float fz = localZ / kTileSize;
    const int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, kTilesPerSide);
    const int z0 = std::clamp(static_cast<int>(std::floor(fz)), 0, kTilesPerSide);
    const int x1 = std::clamp(x0 + 1, 0, kTilesPerSide);
    const int z1 = std::clamp(z0 + 1, 0, kTilesPerSide);
    const float tx = std::clamp(fx - std::floor(fx), 0.0f, 1.0f);
    const float tz = std::clamp(fz - std::floor(fz), 0.0f, 1.0f);
    auto at = [&](int x, int z) {
        const int idx = HeightVertexIndex(x, z);
        return (idx >= 0 && idx < static_cast<int>(hm.size())) ? hm[static_cast<size_t>(idx)] : 0.0f;
    };
    const float h0 = at(x0, z0) * (1.0f - tx) + at(x1, z0) * tx;
    const float h1 = at(x0, z1) * (1.0f - tx) + at(x1, z1) * tx;
    return h0 * (1.0f - tz) + h1 * tz;
}

inline float SampleHeightAtTileCenter(const std::vector<float>& hm, int tx, int tz) {
    float cx = 0.0f, cz = 0.0f;
    TileCenterLocal(tx, tz, cx, cz);
    return SampleHeightBilinear(hm, cx, cz);
}

inline float RegionOffsetX(int regionRy, int centerRy) {
    return static_cast<float>(regionRy - centerRy) * kRegionSize;
}

inline float RegionOffsetZ(int regionRx, int centerRx) {
    return static_cast<float>(regionRx - centerRx) * kRegionSize;
}

} // namespace sro::nav
