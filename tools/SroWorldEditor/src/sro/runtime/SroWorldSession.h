#pragma once
#include <d3d9.h>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <functional>
#include "runtime/RegionCache.h"
#include "assets/AssetResolver.h"
#include "core/ClientLoadProgress.h"
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
    void PrepareClientShell(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath);
    bool StepAssetLoad(ClientLoadProgress& progress);
    bool FinalizeClientOpen(int rx, int ry, ClientLoadProgress& progress);

    // Incremental finalization: splits the region loading into per-neighbor steps
    // so the progress bar can report each navmesh/placement file individually.
    bool BeginFinalizeClientOpen(int rx, int ry, ClientLoadProgress& progress);
    bool StepFinalizeClientOpen(ClientLoadProgress& progress);
    bool IsFinalizeDone() const { return m_finalizePhase == FinalizePhase::Done; }
    bool IsFinalizeIdle() const { return m_finalizePhase == FinalizePhase::Idle; }
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
    AssetResolver& Assets() { return m_assets; }

    int CenterRx() const { return m_centerRx; }
    int CenterRy() const { return m_centerRy; }
    int LoadRadius() const { return m_cache.LoadRadius(); }
    void SetLoadRadius(int r);

    const std::wstring& ClientPath() const { return m_clientPath; }
    bool IsOpen() const { return m_isOpen; }

    bool SavePlacements();
    bool SaveTerrain();
    bool SaveNavmesh();

    void EnsurePlacementCollision(PlacementVM& vm);
    void EagerLoadRegionPlacementCollision(int rx, int ry, std::vector<PlacementVM>& placements);
    void EnsurePlacementModel(PlacementVM& vm);

private:
    void LoadMapInfo();
    void OnPlacementLoaded(int rx, int ry);
    void OnPlacementUnloaded(int rx, int ry);
    void UpdateFinalizeProgress(ClientLoadProgress& progress) const;

    enum class FinalizePhase { Idle, InitRenderer, ScanWorldMap, BeginTerrain, LoadRegions, Done };
    FinalizePhase m_finalizePhase = FinalizePhase::Idle;
    int m_finalizeRx = 0, m_finalizeRy = 0;
    int m_finalizeRegionIdx = 0;
    int m_finalizeTotalSteps = 0;
    std::vector<std::pair<int, int>> m_finalizeRegions;

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
    LPDIRECT3DDEVICE9 m_device = nullptr;
};

} // namespace sro
