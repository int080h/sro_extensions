#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace sro::map {

constexpr int kOriginRy = 135;
constexpr int kOriginRx = 92;
constexpr float kRegionWorldSize = 192.0f;
constexpr float kSceneRegionSize = 1920.0f;
constexpr float kMapPixelsPerRegion = 32.0f;
constexpr float kWorldTilePixels = 128.0f;
constexpr int kRegionsPerWorldTile = 4;

constexpr int kWorldTileMinMx = 46;
constexpr int kWorldTileMaxMx = 180;
constexpr int kWorldTileStep = 4;
constexpr int kWorldTileStartMy = 113;
constexpr int kWorldTileEndMy = 69;

struct SimpleWorldPos {
    float x = 0.0f;
    float z = 0.0f;
};

struct MapPixelPos {
    float x = 0.0f;
    float y = 0.0f;
};

constexpr float kLocalMapGridSize = 256.0f;

struct TexturePixelPos {
    float x = 0.0f;
    float y = 0.0f;
};

struct LocalMapCoordSpace {
    float logicalWidth = 0.0f;
    float logicalHeight = 0.0f;
    float uvMinU = 0.0f;
    float uvMinV = 0.0f;
    float uvMaxU = 1.0f;
    float uvMaxV = 1.0f;
};

inline LocalMapCoordSpace LocalMapCoordSpaceFromDef(int minMx, int maxMx, int minMy, int maxMy,
                                                    int renderWidth, int renderHeight,
                                                    int textureWidth, int textureHeight) {
    LocalMapCoordSpace space;
    space.logicalWidth =
        static_cast<float>(std::max(1, maxMx - minMx + 1)) * kLocalMapGridSize;
    space.logicalHeight =
        static_cast<float>(std::max(1, maxMy - minMy + 1)) * kLocalMapGridSize;
    const int texW = std::max(1, textureWidth > 0 ? textureWidth : renderWidth);
    const int texH = std::max(1, textureHeight > 0 ? textureHeight : renderHeight);
    const int rendW = std::max(1, renderWidth);
    const int rendH = std::max(1, renderHeight);
    // Left/top-aligned crop when texture exceeds render (matches ISRO sub_6FD9C0 mode 2).
    space.uvMinU = 0.0f;
    space.uvMaxU = static_cast<float>(rendW) / static_cast<float>(texW);
    space.uvMinV = 0.0f;
    space.uvMaxV = static_cast<float>(rendH) / static_cast<float>(texH);
    return space;
}

inline TexturePixelPos TexturePixelToNormalized(const TexturePixelPos& tp, float logicalW,
                                                float logicalH) {
    return {tp.x / std::max(1.0f, logicalW), tp.y / std::max(1.0f, logicalH)};
}

inline TexturePixelPos RenderPixelToNormalized(const TexturePixelPos& rp, float renderW,
                                               float renderH) {
    return {rp.x / std::max(1.0f, renderW), rp.y / std::max(1.0f, renderH)};
}

// City inner-map view state (ISRO CWorldMapView / sub_6FFBA0 mode 2 + sub_6FD470).
struct LocalMapViewState {
    float coordWidth = 0.0f;
    float coordHeight = 0.0f;
    float renderWidth = 0.0f;
    float renderHeight = 0.0f;
    float textureWidth = 0.0f;
    float textureHeight = 0.0f;
    int minMx = 0;
    float worldMinX = 0.0f;
    float worldMaxX = 0.0f;
    float worldZNorth = 0.0f;
    float worldZSouth = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    LocalMapCoordSpace uv;
};

inline void ScaleCoordPixelToRender(float& x, float& y, const LocalMapViewState& state) {
    const float cw = state.coordWidth > 0.0f ? state.coordWidth : state.renderWidth;
    const float ch = state.coordHeight > 0.0f ? state.coordHeight : state.renderHeight;
    if (cw > 0.0f && ch > 0.0f &&
        (cw != state.renderWidth || ch != state.renderHeight)) {
        x = x * state.renderWidth / cw;
        y = y * state.renderHeight / ch;
    }
}

inline LocalMapViewState LocalMapViewStateFromDef(int mx0, int my0, int mx1, int my1, int minMx,
                                                  int maxMx, int minMy, int maxMy, int coordWidth,
                                                  int coordHeight, int renderWidth,
                                                  int renderHeight, int textureWidth,
                                                  int textureHeight) {
    LocalMapViewState state;
    state.renderWidth = static_cast<float>(std::max(1, renderWidth));
    state.renderHeight = static_cast<float>(std::max(1, renderHeight));
    state.coordWidth =
        static_cast<float>(std::max(1, coordWidth > 0 ? coordWidth : renderWidth));
    state.coordHeight =
        static_cast<float>(std::max(1, coordHeight > 0 ? coordHeight : renderHeight));
    state.textureWidth =
        static_cast<float>(std::max(1, textureWidth > 0 ? textureWidth : renderWidth));
    state.textureHeight =
        static_cast<float>(std::max(1, textureHeight > 0 ? textureHeight : renderHeight));
    state.minMx = minMx;
    state.uv = LocalMapCoordSpaceFromDef(minMx, maxMx, minMy, maxMy, renderWidth, renderHeight,
                                         textureWidth, textureHeight);
    // sub_6FFBA0 mode 2: raw mx0/my0/mx1/my1 from wlocalmap (file order, not sorted).
    const float v8 = static_cast<float>(mx0 - kOriginRy);
    const float v9 = static_cast<float>(my0 - kOriginRx);
    const float v10 = static_cast<float>(mx1 - kOriginRy);
    const float v11 = static_cast<float>(my1 - kOriginRx);
    state.worldMinX = kRegionWorldSize * v8;
    state.worldMaxX = kRegionWorldSize * (v10 + 1.0f);
    state.worldZNorth = kRegionWorldSize * (v9 + 1.0f);
    state.worldZSouth = kRegionWorldSize * v11;
    const float spanX = std::max(1.0f, state.worldMaxX - state.worldMinX);
    const float spanZ = std::max(1.0f, std::fabs(state.worldZNorth - state.worldZSouth));
    state.scaleX = spanX / state.renderWidth;
    state.scaleY = spanZ / state.renderHeight;
    return state;
}

inline LocalMapViewState LocalMapViewStateFromDef(int minMx, int maxMx, int minMy, int maxMy,
                                                  int renderWidth, int renderHeight,
                                                  int textureWidth, int textureHeight) {
    return LocalMapViewStateFromDef(minMx, maxMy, maxMx, minMy, minMx, maxMx, minMy, maxMy,
                                    renderWidth, renderHeight, renderWidth, renderHeight,
                                    textureWidth, textureHeight);
}

inline float SimpleWorldXFromRegion(int ry, float localX = 0.0f) {
    return kRegionWorldSize * static_cast<float>(ry - kOriginRy) + localX / 10.0f;
}

inline float SimpleWorldZFromRegion(int rx, float localZ = 0.0f) {
    return kRegionWorldSize * static_cast<float>(rx - kOriginRx) + localZ / 10.0f;
}

// Live player on inner city map (sub_6FD470 mode 1 with city view bounds).
inline TexturePixelPos RegionLocalToRenderPixel(int rx, int ry, float localX, float localZ,
                                                const LocalMapViewState& state) {
    const float simpleX = SimpleWorldXFromRegion(ry, localX);
    const float simpleZ = SimpleWorldZFromRegion(rx, localZ);
    const float x = (simpleX - state.worldMinX) / std::max(1e-6f, state.scaleX);
    const float zNorm = (simpleZ - state.worldZSouth) / std::max(1e-6f, state.scaleY);
    return {x, state.renderHeight - zNorm};
}

// Static POI from worldmap_localinfo — PixelX/Y are coord-space (cols 8–9); scaled to render space.
// Region-relative fallback (sub_6FD470 mode 2) when pixel coords exceed the render window.
inline TexturePixelPos LocalPoiToRenderPixel(int targetRx, int targetRy, float pixelX, float pixelY,
                                             float iconHalfW, float iconHalfH,
                                             const LocalMapViewState& state) {
    float useX = pixelX;
    float useY = pixelY;
    ScaleCoordPixelToRender(useX, useY, state);
    const float texW = state.textureWidth > 0.0f ? state.textureWidth : state.renderWidth;
    const float texH = state.textureHeight > 0.0f ? state.textureHeight : state.renderHeight;
    if (texW > state.renderWidth && useX > state.renderWidth && useX <= texW) {
        useX -= (texW - state.renderWidth);
    }
    if (texH > state.renderHeight && useY > state.renderHeight && useY <= texH) {
        useY -= (texH - state.renderHeight);
    }
    const float cx = useX + iconHalfW;
    const float cy = useY + iconHalfH;
    if (targetRx < 0 || targetRy < 0) {
        return {cx, cy};
    }
    if (useX <= state.renderWidth && useY <= state.renderHeight) {
        return {cx, cy};
    }
    const float relRy = static_cast<float>(targetRy - state.minMx);
    const float spanX = state.worldMaxX - state.worldMinX;
    const float x = cx + (kRegionWorldSize * relRy - spanX) / std::max(1e-6f, state.scaleX);
    const float zNorm =
        (kRegionWorldSize * static_cast<float>(targetRx - kOriginRx) - state.worldZSouth) /
        std::max(1e-6f, state.scaleY);
    return {x, state.renderHeight - zNorm};
}

inline int RegionLowByte(int regionWord) { return regionWord & 0xFF; }
inline int RegionHighByte(int regionWord) { return (regionWord >> 8) & 0xFF; }

inline SimpleWorldPos SceneLocalToSimpleWorld(int rx, int ry, float localX, float localZ) {
    return {
        kRegionWorldSize * static_cast<float>(rx - kOriginRx) + localX / 10.0f,
        kRegionWorldSize * static_cast<float>(ry - kOriginRy) + localZ / 10.0f};
}

inline void RegionToDisplay(int rx, int ry, int& displayMx, int& displayMy) {
    displayMx = ry;
    displayMy = rx;
}

inline void DisplayToRegion(int displayMx, int displayMy, int& rx, int& ry) {
    rx = displayMy;
    ry = displayMx;
}

inline float MapCellToWorldX(int displayMx, int minMx) {
    return static_cast<float>(displayMx - minMx) * kMapPixelsPerRegion;
}

inline float MapCellToWorldY(int displayMy, int maxMy) {
    return static_cast<float>(maxMy - displayMy) * kMapPixelsPerRegion;
}

inline MapPixelPos RegionLocalToMapPixel(int rx, int ry, float localX, float localZ,
                                         int minMx, int maxMy) {
    int displayMx = 0, displayMy = 0;
    RegionToDisplay(rx, ry, displayMx, displayMy);
    const float localFracX = (localX / kSceneRegionSize) * kMapPixelsPerRegion;
    const float localFracZ = (localZ / kSceneRegionSize) * kMapPixelsPerRegion;
    MapPixelPos p;
    // MapCellToWorld* is the north-west corner of the region cell; localZ grows south.
    p.x = MapCellToWorldX(displayMx, minMx) + localFracX;
    p.y = MapCellToWorldY(displayMy, maxMy) + kMapPixelsPerRegion - localFracZ;
    return p;
}

inline void WorldPixelToMapCell(float worldX, float worldY, int minMx, int maxMy, int& mx, int& my) {
    mx = minMx + static_cast<int>(std::floor(worldX / kMapPixelsPerRegion));
    my = maxMy - static_cast<int>(std::floor(worldY / kMapPixelsPerRegion));
}

inline float MinimapHudU(float localX) { return localX / kSceneRegionSize; }
inline float MinimapHudV(float localZ) { return localZ / kSceneRegionSize; }

// North-up HUD dot Y (matches RegionLocalToMapPixel: localZ=0 south, 1920 north).
inline float MinimapHudVScreen(float localZ) { return 1.0f - MinimapHudV(localZ); }

// 3x3 corner HUD: col = Ry (east), row = Rx (north, row 0 = highest Rx).
inline void MinimapHudCellToGrid(int tileRx, int tileRy, int centerRx, int centerRy, int& col, int& row) {
    col = tileRy - (centerRy - 1);
    row = (centerRx + 1) - tileRx;
}

inline void MinimapHudNeighborRegion(int drx, int dry, int centerRx, int centerRy, int& rx, int& ry) {
    rx = centerRx + drx;
    ry = centerRy + dry;
}

// City / instance inner-map texture coords (MinMx/MaxMx = Ry, MinMy/MaxMy = Rx).
// POI data uses a fixed 256px-per-region logical grid; localZ grows south from north edge.
inline TexturePixelPos RegionLocalToLocalTexturePixel(int rx, int ry, float localX, float localZ,
                                                      int minMx, int maxMy) {
    const float g = kLocalMapGridSize;
    const float localFracX = (localX / kSceneRegionSize) * g;
    const float localFracZ = (localZ / kSceneRegionSize) * g;
    return {
        static_cast<float>(ry - minMx) * g + localFracX,
        static_cast<float>(maxMy - rx) * g + g - localFracZ};
}

// Map icon rotation from horizontal camera look. Matches ISRO CWorldMapView+38900 (yaw - pi/2);
// mm_sign_character.ddj art points east at angle 0. Negated so spin matches camera turn direction.
inline float CameraHeadingToMapIconRadians(float frontX, float frontZ) {
    constexpr float kHalfPi = 3.14159265f / 2.0f;
    return -(std::atan2(frontX, -frontZ) - kHalfPi);
}

// Inner-city map quad in world-map pixel space (north-anchored, render aspect).
struct LocalMapWorldRect {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

inline LocalMapWorldRect LocalMapWorldRectFromCellBounds(int minMx, int maxMx, int renderWidth,
                                                         int renderHeight, float worldX0,
                                                         float worldY0) {
    LocalMapWorldRect rect;
    rect.x0 = worldX0;
    rect.y0 = worldY0;
    rect.w = static_cast<float>(std::max(1, maxMx - minMx + 1)) * kMapPixelsPerRegion;
    const int rendW = std::max(1, renderWidth);
    const int rendH = std::max(1, renderHeight);
    rect.h = rect.w * static_cast<float>(rendH) / static_cast<float>(rendW);
    return rect;
}

// Region-minimap overlay: height matches 32px-per-region grid (not render aspect).
inline LocalMapWorldRect LocalMapWorldRectGridAligned(int minMx, int maxMx, int minMy, int maxMy,
                                                      float worldX0, float worldY0) {
    LocalMapWorldRect rect;
    rect.x0 = worldX0;
    rect.y0 = worldY0;
    rect.w = static_cast<float>(std::max(1, maxMx - minMx + 1)) * kMapPixelsPerRegion;
    rect.h = static_cast<float>(std::max(1, maxMy - minMy + 1)) * kMapPixelsPerRegion;
    return rect;
}

inline bool ShouldDrawInnerCityPoi(int mapId, int localId) {
    return mapId != 0 && mapId == localId;
}

// ISRO inner-city map: col 2 MarkerKind==1 means always-on label; other kinds use hover tooltip only.
inline bool ShouldShowInnerCityPermanentLabel(int markerKind) {
    return markerKind == 1;
}

// Permanent label text: col 5 Name first, col 4 AreaName fallback (see worldmap_localinfo.txt).
inline bool ShouldDrawInnerCityPermanentLabel(int markerKind, const std::string& label,
                                              const std::string& localMapNameUtf8) {
    if (label.empty() || !ShouldShowInnerCityPermanentLabel(markerKind)) return false;
    if (!localMapNameUtf8.empty() && label == localMapNameUtf8) return false;
    return true;
}

inline bool IsRedundantInnerCityLabel(const std::string& label, const std::string& localMapNameUtf8) {
    return !localMapNameUtf8.empty() && label == localMapNameUtf8;
}

constexpr float kDungeonTilePixels = 128.0f;

inline std::string FormatFloorLabel(char floorKind, int floorIndex) {
    if (floorKind == '\0' || floorIndex <= 0) return {};
    return std::string(1, floorKind) + std::to_string(floorIndex);
}

struct DungeonFloorGroupInput {
    int id = 0;
    std::string nameUtf8;
    std::string groupKey;
    int floorIndex = 0;
    int floorCount = 0;
};

inline std::string DungeonGroupKeyFromFolder(const std::string& folderUtf8) {
    if (folderUtf8.empty()) return {};
    const auto pos = folderUtf8.rfind("_FLOOR");
    if (pos != std::string::npos) return folderUtf8.substr(0, pos);
    return folderUtf8;
}

inline std::vector<int> GroupDungeonFloorIds(const std::vector<DungeonFloorGroupInput>& all,
                                             int firstFloorId) {
    const DungeonFloorGroupInput* seed = nullptr;
    for (const auto& row : all) {
        if (row.id == firstFloorId) {
            seed = &row;
            break;
        }
    }
    if (!seed || seed->floorCount <= 0) return {};

    auto collectMatches = [&](const auto& predicate) {
        std::vector<std::pair<int, int>> matches;
        for (const auto& row : all) {
            if (predicate(row)) matches.emplace_back(row.floorIndex, row.id);
        }
        std::sort(matches.begin(), matches.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        std::vector<int> ids;
        ids.reserve(matches.size());
        for (const auto& m : matches) ids.push_back(m.second);
        return ids;
    };

    if (!seed->groupKey.empty()) {
        std::vector<int> byKey = collectMatches([&](const DungeonFloorGroupInput& row) {
            return row.groupKey == seed->groupKey && row.floorCount == seed->floorCount;
        });
        if (!byKey.empty()) return byKey;
    }

    if (seed->nameUtf8.empty()) return {};

    return collectMatches([&](const DungeonFloorGroupInput& row) {
        return row.nameUtf8 == seed->nameUtf8 && row.floorCount == seed->floorCount;
    });
}

inline LocalMapWorldRect DungeonFloorWorldRectFromRender(int minMx, int maxMy, int renderWidth,
                                                         int renderHeight) {
    LocalMapWorldRect rect;
    rect.x0 = MapCellToWorldX(minMx, minMx);
    rect.y0 = MapCellToWorldY(maxMy, maxMy);
    rect.w = static_cast<float>(std::max(1, renderWidth));
    const int rendW = std::max(1, renderWidth);
    const int rendH = std::max(1, renderHeight);
    rect.h = rect.w * static_cast<float>(rendH) / static_cast<float>(rendW);
    return rect;
}

struct DungeonFloorLayout {
    float worldW = 0.0f;
    float worldH = 0.0f;
    float tileScaleX = 1.0f;
    float tileScaleY = 1.0f;
};

inline DungeonFloorLayout DungeonFloorLayoutFromDef(int renderWidth, int renderHeight, int tileCols,
                                                    int tileRows, int minMx = 0, int maxMx = 0,
                                                    int minMy = 0, int maxMy = 0) {
    const int cols = tileCols > 0 ? tileCols : (maxMx >= minMx ? maxMx - minMx + 1 : 1);
    const int rows = tileRows > 0 ? tileRows : (maxMy >= minMy ? maxMy - minMy + 1 : 1);
    const float tileGridW =
        static_cast<float>(std::max(1, cols)) * kDungeonTilePixels;
    const float tileGridH =
        static_cast<float>(std::max(1, rows)) * kDungeonTilePixels;

    DungeonFloorLayout layout;
    layout.worldW = static_cast<float>(std::max(1, renderWidth));
    const int rendW = std::max(1, renderWidth);
    const int rendH = std::max(1, renderHeight);
    layout.worldH = layout.worldW * static_cast<float>(rendH) / static_cast<float>(rendW);
    layout.tileScaleX = layout.worldW / tileGridW;
    layout.tileScaleY = layout.worldH / tileGridH;
    return layout;
}

} // namespace sro::map
