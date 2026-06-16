#pragma once
#include "world/RegionBundle.h"
#include "core/RegionId.h"
#include "index/ObjectIndex.h"
#include "index/Tile2DIndex.h"
#include "io/PathHelpers.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include "parsers/MapMeshParser.h"
#include "parsers/MapPlacementParser.h"
#include "parsers/NavMeshParser.h"
#include <map>
#include <deque>
#include <string>
#include <algorithm>
#include <cmath>

namespace sro {

class RegionCache {
public:
    void SetClientPath(const std::wstring& path) { m_clientPath = path; }
    const std::wstring& ClientPath() const { return m_clientPath; }

    void SetLoadRadius(int radius) { m_loadRadius = std::max(0, radius); }
    int LoadRadius() const { return m_loadRadius; }

    RegionBundle& GetOrCreate(const RegionCoord& coord);
    const RegionBundle* Find(const RegionCoord& coord) const;
    RegionBundle* FindMutable(const RegionCoord& coord);

    void LoadRegionGrid(int centerRx, int centerRy);
    void EvictOutsideRadius(int centerRx, int centerRy);

    const std::map<RegionCoord, RegionBundle>& All() const { return m_bundles; }

private:
    void LoadTerrain(RegionBundle& bundle);
    void LoadPlacements(RegionBundle& bundle);
    void LoadNavMesh(RegionBundle& bundle);
    void TouchHistory(const RegionCoord& coord);

    std::wstring m_clientPath;
    int m_loadRadius = 2;
    std::map<RegionCoord, RegionBundle> m_bundles;
    std::deque<RegionCoord> m_history;
};

inline RegionBundle& RegionCache::GetOrCreate(const RegionCoord& coord) {
    auto it = m_bundles.find(coord);
    if (it != m_bundles.end()) return it->second;

    RegionBundle bundle;
    bundle.coord = coord;
    m_bundles[coord] = bundle;
    return m_bundles[coord];
}

inline const RegionBundle* RegionCache::Find(const RegionCoord& coord) const {
    auto it = m_bundles.find(coord);
    return it != m_bundles.end() ? &it->second : nullptr;
}

inline RegionBundle* RegionCache::FindMutable(const RegionCoord& coord) {
    auto it = m_bundles.find(coord);
    return it != m_bundles.end() ? &it->second : nullptr;
}

inline void RegionCache::LoadTerrain(RegionBundle& bundle) {
    if (bundle.terrainState == RegionLoadState::Loaded || bundle.terrainState == RegionLoadState::Missing) return;

    auto path = MapMeshPath(m_clientPath, bundle.coord);
    bundle.terrainSourcePath = path;
    if (!FileExists(path)) {
        bundle.terrainState = RegionLoadState::Missing;
        bundle.terrainError = "Terrain file not found";
        Logger::Instance().Warning("Missing terrain: " + ToNarrow(path));
        return;
    }

    formats::MapMesh mesh;
    auto result = MapMeshParser::Read(path, mesh);
    if (!result.ok) {
        bundle.terrainState = RegionLoadState::Failed;
        bundle.terrainError = result.error;
        Logger::Instance().Error("Failed terrain: " + ToNarrow(path) + " - " + result.error);
        return;
    }

    bundle.terrain = std::move(mesh);
    bundle.terrainState = RegionLoadState::Loaded;
}

inline void RegionCache::LoadPlacements(RegionBundle& bundle) {
    if (bundle.placementState == RegionLoadState::Loaded || bundle.placementState == RegionLoadState::Missing) return;

    auto o2 = MapPlacementO2Path(m_clientPath, bundle.coord);
    auto o = MapPlacementOPath(m_clientPath, bundle.coord);
    std::wstring path;
    if (FileExists(o2)) path = o2;
    else if (FileExists(o)) path = o;
    else {
        bundle.placementState = RegionLoadState::Missing;
        bundle.placements = formats::MapPlacements();
        bundle.placementError = "Placement file not found";
        return;
    }

    bundle.placementSourcePath = path;
    formats::MapPlacements placements;
    auto result = MapPlacementParser::Read(path, placements);
    if (!result.ok) {
        bundle.placementState = RegionLoadState::Failed;
        bundle.placementError = result.error;
        Logger::Instance().Error("Failed placements: " + ToNarrow(path) + " - " + result.error);
        return;
    }

    // Set RegionID for legacy .o objects that lack it
    if (placements.Format == formats::PlacementFormat::LegacyO) {
        uint16_t rid = RegionId::FromCoord(bundle.coord).value;
        for (int z = 0; z < 6; ++z) {
            for (int x = 0; x < 6; ++x) {
                for (auto& obj : placements.Blocks[z][x][0]) {
                    if (obj.RegionID == 0) obj.RegionID = rid;
                }
            }
        }
    }

    bundle.placements = std::move(placements);
    bundle.placementState = RegionLoadState::Loaded;
}

inline void RegionCache::LoadNavMesh(RegionBundle& bundle) {
    if (bundle.navState == RegionLoadState::Loaded || bundle.navState == RegionLoadState::Missing || bundle.navState == RegionLoadState::Failed) return;

    auto path = NavMeshPath(m_clientPath, bundle.coord);
    bundle.navSourcePath = path;
    if (!FileExists(path)) {
        bundle.navState = RegionLoadState::Missing;
        bundle.navError = "Navmesh file not found";
        return;
    }

    formats::NavMesh nav;
    auto result = NavMeshParser::Read(path, nav);
    if (!result.ok) {
        bundle.navState = RegionLoadState::Failed;
        bundle.navError = result.error;
        Logger::Instance().Error("Failed navmesh: " + ToNarrow(path) + " - " + result.error);
        return;
    }

    bundle.navMesh = std::move(nav);
    bundle.navState = RegionLoadState::Loaded;
}

inline void RegionCache::TouchHistory(const RegionCoord& coord) {
    auto it = std::find(m_history.begin(), m_history.end(), coord);
    if (it != m_history.end()) m_history.erase(it);
    m_history.push_back(coord);
}

inline void RegionCache::LoadRegionGrid(int centerRx, int centerRy) {
    for (int dy = -m_loadRadius; dy <= m_loadRadius; ++dy) {
        for (int dx = -m_loadRadius; dx <= m_loadRadius; ++dx) {
            RegionCoord coord(centerRx + dx, centerRy + dy);
            auto& bundle = GetOrCreate(coord);
            LoadTerrain(bundle);
            LoadPlacements(bundle);
            LoadNavMesh(bundle);
            TouchHistory(coord);
        }
    }
    EvictOutsideRadius(centerRx, centerRy);
}

inline void RegionCache::EvictOutsideRadius(int centerRx, int centerRy) {
    const int keepMargin = m_loadRadius + 16;
    for (auto it = m_bundles.begin(); it != m_bundles.end(); ) {
        int drx = std::abs(static_cast<int>(it->first.rx) - centerRx);
        int dry = std::abs(static_cast<int>(it->first.ry) - centerRy);
        if (drx > keepMargin || dry > keepMargin) {
            it = m_bundles.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace sro
