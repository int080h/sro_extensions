#pragma once
#include <d3d9.h>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <functional>
#include "runtime/RegionCache.h"
#include "assets/AssetResolver.h"
#include "render/SceneRenderer.h"
#include "world/WorldBitmap.h"
#include "core/RegionCoord.h"
#include "core/SceneSpace.h"
#include "core/Constants.h"
#include "parsers/MapProjectParser.h"
#include "io/PathHelpers.h"
#include "runtime/RegionManager.h"
#include "PlacementVM.h"
#include "core/Logger.h"

namespace sro {

class SroWorldSession {
public:
    bool OpenClient(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath);
    void Shutdown();

    bool LoadRegion(int rx, int ry);
    void UpdateCameraRegion(int& inOutRx, int& inOutRy, Vector3& cameraPos);

    void CollectPlacements(std::vector<PlacementVM>& out) const;
    void BuildScenePlacements(const std::vector<PlacementVM>& source,
                              const std::function<bool(int16_t)>& isSelected,
                              std::vector<ScenePlacement>& out) const;

    SceneRenderer& Renderer() { return m_renderer; }
    RegionManager* LegacyRegionManager() { return m_regionManager.get(); }
    RegionCache& Cache() { return m_cache; }
    const WorldBitmap& MapProject() const { return m_mapProject; }
    const AssetResolver& Assets() const { return m_assets; }

    int CenterRx() const { return m_centerRx; }
    int CenterRy() const { return m_centerRy; }
    int LoadRadius() const { return m_cache.LoadRadius(); }
    void SetLoadRadius(int r);

    const std::wstring& ClientPath() const { return m_clientPath; }
    bool IsOpen() const { return m_isOpen; }

    bool SavePlacements();
    bool SaveTerrain();
    bool SaveNavmesh();

private:
    void LoadMapInfo();
    void OnPlacementLoaded(int rx, int ry);
    void OnPlacementUnloaded(int rx, int ry);

    bool m_isOpen = false;
    std::wstring m_clientPath;
    int m_centerRx = DEFAULT_REGION_RX;
    int m_centerRy = DEFAULT_REGION_RY;

    SceneRenderer m_renderer;
    RegionCache m_cache;
    AssetResolver m_assets;
    WorldBitmap m_mapProject;

    std::unique_ptr<RegionManager> m_regionManager;
    mutable std::vector<PlacementVM> m_placements;
    std::set<std::pair<int, int>> m_gpuPlacementRegions;
};

} // namespace sro
