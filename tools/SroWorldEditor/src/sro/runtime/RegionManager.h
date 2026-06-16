#pragma once
#include <map>
#include <set>
#include <deque>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include "formats/MapFormats.h"
#include "formats/NavMeshFormat.h"
#include "parsers/MapPlacementParser.h"
#include "parsers/NavMeshParser.h"
#include "parsers/MapMeshParser.h"
#include "io/PathHelpers.h"
#include "core/SceneSpace.h"
#include "rendering/RenderManager.h"
#include "core/Log.h"
#include "core/Utils.h"
#include "core/EditorTypes.h"
#include "core/EditorAction.h"
#include "core/RegionCoord.h"
#include "core/FileSystem.h"
#include "cache/ClientDataCache.h"

class RegionManager {
private:
    std::wstring m_clientPath;
    RenderManager* m_renderManager = nullptr;

    int m_currentRx = 0;
    int m_currentRy = 0;
    int m_loadRadius = 1;

    std::set<std::pair<int, int>> m_loadedPlacementRegions;
    std::set<std::pair<int, int>> m_loadedNvmRegions;
    std::map<std::pair<int, int>, std::unique_ptr<sro::formats::NavMesh>> m_loadedNavMeshes;
    std::deque<std::pair<int, int>> m_loadedRegionsHistory;

    std::map<std::pair<int, int>, sro::formats::MapPlacements> m_loadedPlacements;
    std::map<std::pair<int, int>, std::wstring> m_placementsPaths;
    std::wstring m_currentNvmPath;

    std::vector<std::unique_ptr<class EditorAction>> m_undoStack;
    std::vector<std::unique_ptr<class EditorAction>> m_redoStack;

public:
    RegionManager() = default;

    void PushAction(std::unique_ptr<class EditorAction> action) {
        action->Redo(this);
        m_undoStack.push_back(std::move(action));
        m_redoStack.clear();
    }

    void Undo() {
        if (!m_undoStack.empty()) {
            auto action = std::move(m_undoStack.back());
            m_undoStack.pop_back();
            action->Undo(this);
            m_redoStack.push_back(std::move(action));
        }
    }

    void Redo() {
        if (!m_redoStack.empty()) {
            auto action = std::move(m_redoStack.back());
            m_redoStack.pop_back();
            action->Redo(this);
            m_undoStack.push_back(std::move(action));
        }
    }

    void ClearHistory() {
        m_undoStack.clear();
        m_redoStack.clear();
    }

    void Initialize(const std::wstring& clientPath, RenderManager* renderManager) {
        m_clientPath = clientPath;
        m_renderManager = renderManager;
    }

    int GetCurrentRx() const { return m_currentRx; }
    int GetCurrentRy() const { return m_currentRy; }
    int GetLoadRadius() const { return m_loadRadius; }
    void SetLoadRadius(int radius) { m_loadRadius = radius; }

    const std::map<std::pair<int, int>, std::unique_ptr<sro::formats::NavMesh>>& GetLoadedNavMeshes() const {
        return m_loadedNavMeshes;
    }

    RenderManager* GetRenderManager() const { return m_renderManager; }

    std::set<std::pair<int, int>>& GetLoadedPlacementRegions() {
        return m_loadedPlacementRegions;
    }

    sro::formats::NavMesh* GetNavMesh(int rx, int ry) {
        auto it = m_loadedNavMeshes.find({rx, ry});
        if (it != m_loadedNavMeshes.end()) return it->second.get();
        return nullptr;
    }

    sro::formats::NavMesh* GetCenterNavMesh() {
        return GetNavMesh(m_currentRx, m_currentRy);
    }

    std::map<std::pair<int, int>, sro::formats::MapPlacements>& GetLoadedPlacements() {
        return m_loadedPlacements;
    }

    sro::formats::MapPlacements* GetPlacements(int rx, int ry) {
        rx = std::clamp(rx, 0, 255);
        ry = std::clamp(ry, 0, 255);
        auto pair = std::make_pair(rx, ry);

        auto it = m_loadedPlacements.find(pair);
        if (it != m_loadedPlacements.end()) {
            return &it->second;
        }

        std::wstring path = sro::ResolvePlacementPath(m_clientPath, sro::RegionCoord(rx, ry));
        m_placementsPaths[pair] = sro::MapPlacementO2Path(m_clientPath, sro::RegionCoord(rx, ry));

        if (FileExists(path)) {
            sro::formats::MapPlacements placements;
            if (sro::ClientDataCache::Instance().LoadMapPlacements(path, placements)) {
                m_loadedPlacements[pair] = std::move(placements);
                m_placementsPaths[pair] = path;
                return &m_loadedPlacements[pair];
            }
        }

        m_loadedPlacements[pair] = sro::formats::MapPlacements();
        return &m_loadedPlacements[pair];
    }

    sro::formats::MapPlacements* GetCenterPlacements() {
        return GetPlacements(m_currentRx, m_currentRy);
    }

    const std::wstring& GetCurrentPlacementsPath() const {
        auto it = m_placementsPaths.find({m_currentRx, m_currentRy});
        if (it != m_placementsPaths.end()) return it->second;
        static std::wstring dummy;
        return dummy;
    }

    const std::wstring& GetCurrentNvmPath() const {
        return m_currentNvmPath;
    }

    bool IsRegionLoaded(int rx, int ry) const {
        return m_loadedNvmRegions.count({rx, ry}) > 0;
    }

    float GetHeightAt(float x, float z) {
        sro::LocalPos lp = sro::SceneSpace::SceneToLocal(x, z, m_currentRx, m_currentRy);
        sro::formats::NavMesh* nav = GetNavMesh(lp.rx, lp.ry);
        if (nav && m_renderManager && m_renderManager->GetNvmRenderer()) {
            return m_renderManager->GetNvmRenderer()->GetHeight(lp.localX, lp.localZ, nav->HeightMap);
        }
        return 0.0f;
    }

    bool RaycastTerrain(const Vector3& rayPos, const Vector3& rayDir, float maxDist, float step, Vector3& outHitPos) {
        for (float dist = 0.0f; dist < maxDist; dist += step) {
            Vector3 currentPos = rayPos + rayDir * dist;
            float height = GetHeightAt(currentPos.x, currentPos.z);
            if (currentPos.y <= height) {
                outHitPos = currentPos;
                return true;
            }
        }
        return false;
    }

    std::vector<NavmeshFlagDelta> PaintNavmesh(float x, float z, bool setBlocked, float brushSize, int paintMode) {
        std::vector<NavmeshFlagDelta> modifiedTiles;
        sro::LocalPos lp = sro::SceneSpace::SceneToLocal(x, z, m_currentRx, m_currentRy);
        sro::formats::NavMesh* nav = GetNavMesh(lp.rx, lp.ry);
        if (!nav) return modifiedTiles;

        int hitTx = static_cast<int>(lp.localX / 20.0f);
        int hitTz = static_cast<int>(lp.localZ / 20.0f);

        if (hitTx >= 0 && hitTx < 96 && hitTz >= 0 && hitTz < 96) {
            int brushRadius = static_cast<int>(brushSize) - 1;
            bool modified = false;

            for (int dz = -brushRadius; dz <= brushRadius; ++dz) {
                for (int dx = -brushRadius; dx <= brushRadius; ++dx) {
                    int tx = hitTx + dx;
                    int tz = hitTz + dz;

                    if (tx >= 0 && tx < 96 && tz >= 0 && tz < 96) {
                        int tileIdx = tz * 96 + tx;
                        if (tileIdx < static_cast<int>(nav->TileMap.size())) {
                            auto& tile = nav->TileMap[tileIdx];
                            uint16_t oldFlags = tile.Flags;
                            if (setBlocked) {
                                tile.Flags |= 0x0001;
                            } else {
                                tile.Flags &= 0xFFFE;
                            }
                            if (tile.Flags != oldFlags) {
                                modifiedTiles.push_back({ tileIdx, oldFlags, tile.Flags });
                                modified = true;
                            }
                        }
                    }
                }
            }

            if (modified && m_renderManager && m_renderManager->GetNvmRenderer()) {
                m_renderManager->GetNvmRenderer()->RebuildNavmeshBuffers(nav, lp.rx, lp.ry);
            }
        }
        return modifiedTiles;
    }

    void LoadRegion(int rx, int ry, std::function<void(int, int)> placementsLoader, std::function<void(int, int)> placementsUnloader) {
        rx = std::clamp(rx, 0, 255);
        ry = std::clamp(ry, 0, 255);

        m_currentRx = rx;
        m_currentRy = ry;

        if (m_renderManager && m_renderManager->GetTerrainRenderer()) {
            m_renderManager->GetTerrainRenderer()->LoadRegion(rx, ry, m_loadRadius);
        }

        for (int dy = -m_loadRadius; dy <= m_loadRadius; ++dy) {
            for (int dx = -m_loadRadius; dx <= m_loadRadius; ++dx) {
                int targetRx = rx + dx;
                int targetRy = ry + dy;

                if (placementsLoader) {
                    placementsLoader(targetRx, targetRy);
                }

                if (m_loadedNvmRegions.count({targetRx, targetRy}) == 0) {
                    std::wstring nvmPath = sro::NavMeshPath(m_clientPath, sro::RegionCoord(targetRx, targetRy));

                    if (FileExists(nvmPath)) {
                        auto nav = std::make_unique<sro::formats::NavMesh>();
                        if (sro::ClientDataCache::Instance().LoadNavMesh(nvmPath, *nav)) {
                            LogMsgW(L"  [Navmesh] Loaded: " + nvmPath);
                            if (m_renderManager && m_renderManager->GetNvmRenderer()) {
                                m_renderManager->GetNvmRenderer()->LoadNavmesh(*nav, targetRx, targetRy);
                            }
                            m_loadedNavMeshes[{targetRx, targetRy}] = std::move(nav);
                        } else {
                            LogMsgW(L"  [Navmesh] Failed to parse: " + nvmPath);
                        }
                    }
                    m_loadedNvmRegions.insert({targetRx, targetRy});
                    TrackAndEnforceCacheLimit(targetRx, targetRy, placementsUnloader);
                }
            }
        }

        GetPlacements(rx, ry);

        m_currentNvmPath = sro::NavMeshPath(m_clientPath, sro::RegionCoord(rx, ry));
    }

    bool SavePlacements() {
        bool success = true;
        for (auto& pair : m_loadedPlacements) {
            auto itPath = m_placementsPaths.find(pair.first);
            if (itPath != m_placementsPaths.end() && !itPath->second.empty()) {
                if (!sro::MapPlacementParser::Write(itPath->second, pair.second).ok) {
                    success = false;
                }
            }
        }
        return success;
    }

    bool SaveTerrain() {
        if (m_renderManager && m_renderManager->GetTerrainRenderer()) {
            auto* terrain = m_renderManager->GetTerrainRenderer();
            auto* mesh = terrain->GetMapMesh(m_currentRx, m_currentRy);
            if (mesh) {
                std::wstring mPath = sro::MapMeshPath(m_clientPath, sro::RegionCoord(m_currentRx, m_currentRy));
                return sro::MapMeshParser::Write(mPath, *mesh).ok;
            }
        }
        return false;
    }

    bool SaveNavmesh() {
        sro::formats::NavMesh* nav = GetCenterNavMesh();
        if (nav && !m_currentNvmPath.empty()) {
            return sro::NavMeshParser::Write(m_currentNvmPath, *nav).ok;
        }
        return false;
    }

    void UnloadRegionFromCache(int rx, int ry, std::function<void(int, int)> placementsUnloader) {
        if (std::abs(rx - m_currentRx) <= m_loadRadius && std::abs(ry - m_currentRy) <= m_loadRadius) {
            return;
        }

        if (m_renderManager && m_renderManager->GetTerrainRenderer()) {
            m_renderManager->GetTerrainRenderer()->UnloadRegion(rx, ry);
        }

        if (placementsUnloader) {
            placementsUnloader(rx, ry);
        }
        m_loadedPlacementRegions.erase({rx, ry});
        m_loadedPlacements.erase({rx, ry});
        m_placementsPaths.erase({rx, ry});

        if (m_renderManager && m_renderManager->GetNvmRenderer()) {
            m_renderManager->GetNvmRenderer()->UnloadNavmesh(rx, ry);
        }
        m_loadedNvmRegions.erase({rx, ry});
        m_loadedNavMeshes.erase({rx, ry});
    }

    void TrackAndEnforceCacheLimit(int rx, int ry, std::function<void(int, int)> placementsUnloader) {
        auto it = std::find(m_loadedRegionsHistory.begin(), m_loadedRegionsHistory.end(), std::make_pair(rx, ry));
        if (it != m_loadedRegionsHistory.end()) {
            m_loadedRegionsHistory.erase(it);
        }
        m_loadedRegionsHistory.push_back({rx, ry});

        int activeRegionCount = (2 * m_loadRadius + 1) * (2 * m_loadRadius + 1);
        size_t cacheLimit = static_cast<size_t>(activeRegionCount + 16);

        for (auto historyIt = m_loadedRegionsHistory.begin(); historyIt != m_loadedRegionsHistory.end(); ) {
            if (m_loadedRegionsHistory.size() <= cacheLimit) {
                break;
            }

            int hRx = historyIt->first;
            int hRy = historyIt->second;

            if (std::abs(hRx - m_currentRx) > m_loadRadius || std::abs(hRy - m_currentRy) > m_loadRadius) {
                UnloadRegionFromCache(hRx, hRy, placementsUnloader);
                historyIt = m_loadedRegionsHistory.erase(historyIt);
            } else {
                ++historyIt;
            }
        }
    }

    bool HasLoadedPlacementRegion(int rx, int ry) const {
        return m_loadedPlacementRegions.count({rx, ry}) > 0;
    }

    void InsertLoadedPlacementRegion(int rx, int ry) {
        m_loadedPlacementRegions.insert({rx, ry});
    }
};
