#include "SroWorldSession.h"
#include "cache/ClientDataCache.h"
#include "core/FileSystem.h"
#include "core/RegionId.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace sro {

bool SroWorldSession::OpenClient(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath) {
    if (!device || !FileExists(clientPath)) return false;

    PrepareClientShell(device, clientPath);
    ClientLoadProgress progress;
    while (StepAssetLoad(progress)) {}

    progress = {};
    return FinalizeClientOpen(m_centerRx, m_centerRy, progress);
}

void SroWorldSession::PrepareClientShell(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath) {
    if (!device || !FileExists(clientPath)) return;

    Shutdown();
    m_device = device;
    m_clientPath = clientPath;
    m_cache.SetClientPath(clientPath);
    ClientDataCache::Instance().SetClientPath(clientPath);
    m_assets.ResetForLoad(clientPath);
    m_finalizePhase = FinalizePhase::Idle;
    m_finalizeRegionIdx = 0;
    m_finalizeRegions.clear();
}

bool SroWorldSession::StepAssetLoad(ClientLoadProgress& progress) {
    if (!m_device || m_clientPath.empty()) return false;
    return m_assets.StepLoad(progress);
}

bool SroWorldSession::FinalizeClientOpen(int rx, int ry, ClientLoadProgress& progress) {
    if (!m_device || m_clientPath.empty()) return false;

    progress.stage = "Initializing renderer...";
    progress.fraction = 0.95f;
    m_renderer.Initialize(m_device, m_clientPath);
    m_regionManager = std::make_unique<RegionManager>();
    m_regionManager->Initialize(m_clientPath, m_renderer.GetRenderManager());
    SetLoadRadius(1);

    LoadMapInfo();
    if (auto* rm = m_renderer.GetRenderManager()) {
        if (auto* wmc = rm->GetWorldMapControl()) {
            wmc->ActiveRegions = m_mapProject.ActiveRegions;
            wmc->ScanRegionFiles(m_clientPath);
            wmc->MapStyle = 0;
            wmc->CurrentRx = m_centerRx;
            wmc->CurrentRy = m_centerRy;
        }
    }

    progress.stage = "Loading world region...";
    m_isOpen = true;
    if (!LoadRegion(rx, ry)) {
        m_isOpen = false;
        progress.failed = true;
        progress.error = "Failed to load region " + std::to_string(rx) + "," + std::to_string(ry);
        return false;
    }

    progress.stage = "Ready";
    progress.done = true;
    progress.fraction = 1.f;
    Logger::Instance().Info("SRO client opened: " + ToNarrow(m_clientPath));
    return true;
}

bool SroWorldSession::BeginFinalizeClientOpen(int rx, int ry, ClientLoadProgress& progress) {
    if (!m_device || m_clientPath.empty()) return false;

    m_centerRx = std::clamp(rx, 0, 255);
    m_centerRy = std::clamp(ry, 0, 255);
    m_finalizeRx = m_centerRx;
    m_finalizeRy = m_centerRy;
    m_finalizeRegionIdx = 0;
    m_finalizeRegions.clear();
    m_finalizePhase = FinalizePhase::InitRenderer;
    m_isOpen = true;

    int radius = 1;
    m_finalizeTotalSteps = 3 + (2 * radius + 1) * (2 * radius + 1);
    progress.step = 0;
    progress.totalSteps = m_finalizeTotalSteps;
    progress.stage = "Initializing renderer...";
    progress.fraction = 0.85f;
    return true;
}

void SroWorldSession::UpdateFinalizeProgress(ClientLoadProgress& progress) const {
    if (m_finalizeTotalSteps <= 0) return;
    progress.step = (std::min)(m_finalizeTotalSteps, m_finalizeRegionIdx + 1);
    progress.totalSteps = m_finalizeTotalSteps;
    progress.fraction = 0.85f + 0.15f * static_cast<float>(progress.step) / static_cast<float>(m_finalizeTotalSteps);
}

bool SroWorldSession::StepFinalizeClientOpen(ClientLoadProgress& progress) {
    if (m_finalizePhase == FinalizePhase::Done || m_finalizePhase == FinalizePhase::Idle)
        return false;

    switch (m_finalizePhase) {
    case FinalizePhase::InitRenderer: {
        progress.stage = "Initializing renderer...";
        m_renderer.Initialize(m_device, m_clientPath);
        m_regionManager = std::make_unique<RegionManager>();
        m_regionManager->Initialize(m_clientPath, m_renderer.GetRenderManager());
        SetLoadRadius(1);
        LoadMapInfo();
        m_finalizePhase = FinalizePhase::ScanWorldMap;
        m_finalizeRegionIdx = 0;
        UpdateFinalizeProgress(progress);
        return true;
    }
    case FinalizePhase::ScanWorldMap: {
        progress.stage = "Scanning world map...";
        if (auto* rm = m_renderer.GetRenderManager()) {
            if (auto* wmc = rm->GetWorldMapControl()) {
                wmc->ActiveRegions = m_mapProject.ActiveRegions;
                wmc->ScanRegionFiles(m_clientPath);
                wmc->MapStyle = 0;
                wmc->CurrentRx = m_centerRx;
                wmc->CurrentRy = m_centerRy;
            }
        }
        m_finalizePhase = FinalizePhase::BeginTerrain;
        m_finalizeRegionIdx = 1;
        UpdateFinalizeProgress(progress);
        return true;
    }
    case FinalizePhase::BeginTerrain: {
        progress.stage = "Loading terrain...";
        int radius = m_regionManager->GetLoadRadius();
        m_finalizeRegions.clear();
        m_finalizeRegions.reserve((2 * radius + 1) * (2 * radius + 1));
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx)
                m_finalizeRegions.push_back({m_centerRx + dx, m_centerRy + dy});
        m_finalizeRegionIdx = 2;
        m_finalizeTotalSteps = 3 + (int)m_finalizeRegions.size();
        m_regionManager->BeginLoadRegion(m_centerRx, m_centerRy);
        m_finalizePhase = FinalizePhase::LoadRegions;
        UpdateFinalizeProgress(progress);
        return true;
    }
    case FinalizePhase::LoadRegions: {
        int total = (int)m_finalizeRegions.size();
        if (m_finalizeRegionIdx - 2 >= total) {
            m_regionManager->FinishLoadRegion(m_finalizeRx, m_finalizeRy);
            m_finalizePhase = FinalizePhase::Done;
            progress.stage = "Ready";
            progress.done = true;
            progress.active = false;
            progress.step = m_finalizeTotalSteps;
            progress.totalSteps = m_finalizeTotalSteps;
            progress.fraction = 1.f;
            Logger::Instance().Info("SRO client opened: " + ToNarrow(m_clientPath));
            return false;
        }

        auto [trx, try_] = m_finalizeRegions[m_finalizeRegionIdx - 2];
        wchar_t nvmName[32];
        swprintf_s(nvmName, L"nv_%02x%02x.nvm", trx, try_);
        progress.stage = "Loading region (" + std::to_string(trx) + "," + std::to_string(try_) +
                         "): " + ToNarrow(nvmName) + "...";
        progress.step = m_finalizeRegionIdx + 1;
        progress.totalSteps = m_finalizeTotalSteps;
        progress.fraction = 0.85f + 0.15f * static_cast<float>(m_finalizeRegionIdx + 1) / static_cast<float>(m_finalizeTotalSteps);

        m_regionManager->StepLoadNeighbor(trx, try_,
            [this](int prx, int pry) { OnPlacementLoaded(prx, pry); },
            [this](int prx, int pry) { OnPlacementUnloaded(prx, pry); });

        ++m_finalizeRegionIdx;
        return true;
    }
    default:
        return false;
    }
}

void SroWorldSession::Shutdown() {
    m_placements.clear();
    m_gpuPlacementRegions.clear();
    m_regionManager.reset();
    m_renderer.Shutdown();
    m_isOpen = false;
    m_finalizePhase = FinalizePhase::Idle;
    m_finalizeRegionIdx = 0;
    m_finalizeRegions.clear();
}

void SroWorldSession::LoadMapInfo() {
    auto path = NavMeshMapInfoPath(m_clientPath);
    if (FileExists(path)) {
        if (MapProjectParser::Read(path, m_mapProject).ok) {
            Logger::Instance().Info("Loaded mapinfo.mfo: " + std::to_string(m_mapProject.ActiveRegions.size()) + " active regions");
            return;
        }
    }
    path = MapInfoPath(m_clientPath);
    if (FileExists(path)) {
        if (MapProjectParser::Read(path, m_mapProject).ok) {
            Logger::Instance().Info("Loaded mapinfo.mfo (Map/): " + std::to_string(m_mapProject.ActiveRegions.size()) + " active regions");
            return;
        }
    }
    Logger::Instance().Warning("mapinfo.mfo not found");
}

void SroWorldSession::SetLoadRadius(int r) {
    m_cache.SetLoadRadius(r);
    if (m_regionManager) m_regionManager->SetLoadRadius(r);
}

bool SroWorldSession::LoadRegion(int rx, int ry) {
    if (!m_isOpen || !m_regionManager) return false;

    m_centerRx = std::clamp(rx, 0, 255);
    m_centerRy = std::clamp(ry, 0, 255);

    m_placements.clear();
    m_gpuPlacementRegions.clear();
    m_regionManager->LoadRegion(m_centerRx, m_centerRy,
        [this](int targetRx, int targetRy) { OnPlacementLoaded(targetRx, targetRy); },
        [this](int targetRx, int targetRy) { OnPlacementUnloaded(targetRx, targetRy); });

    Logger::Instance().Info("Loaded region " + std::to_string(m_centerRx) + "," + std::to_string(m_centerRy));

    if (auto* rm = m_renderer.GetRenderManager()) {
        if (auto* wmc = rm->GetWorldMapControl()) {
            wmc->CurrentRx = m_centerRx;
            wmc->CurrentRy = m_centerRy;
        }
    }
    return true;
}

void SroWorldSession::OnPlacementLoaded(int rx, int ry) {
    if (m_gpuPlacementRegions.count({rx, ry})) return;
    m_gpuPlacementRegions.insert({rx, ry});

    auto* placements = m_regionManager->GetPlacements(rx, ry);
    if (!placements) return;

    std::set<std::tuple<uint32_t, int16_t, int, int, int>> seenPlacements;
    auto quantizePos = [](float value) {
        return static_cast<int>(std::lround(value * 100.0f));
    };

    const int lodCount = placements->LodCount();
    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            for (int lod = 0; lod < lodCount; ++lod) {
                int idx = 0;
                for (const auto& obj : placements->Blocks[z][x][lod]) {
                    const auto key = std::make_tuple(obj.ObjID, obj.UID,
                        quantizePos(obj.PosX), quantizePos(obj.PosY), quantizePos(obj.PosZ));
                    if (!seenPlacements.insert(key).second) {
                        ++idx;
                        continue;
                    }

                    PlacementVM vm;
                    vm.Object = obj;
                    vm.BsrPath = m_assets.ResolveObjectBsr(obj.ObjID);
                    if (vm.BsrPath.empty()) {
                        if (auto* rm = m_renderer.GetRenderManager()) {
                            if (auto* mesh = rm->GetMeshRenderer())
                                vm.BsrPath = mesh->GetModelBsrPath(obj.ObjID);
                        }
                    }
                    vm.BlockZ = z;
                    vm.BlockX = x;
                    vm.Lod = lod;
                    vm.Index = idx++;
                    vm.LoadedRx = rx;
                    vm.LoadedRy = ry;

                    m_placements.push_back(vm);
                }
            }
        }
    }
}

void SroWorldSession::OnPlacementUnloaded(int rx, int ry) {
    m_gpuPlacementRegions.erase({rx, ry});
    m_placements.erase(std::remove_if(m_placements.begin(), m_placements.end(),
        [rx, ry](const PlacementVM& p) { return p.LoadedRx == rx && p.LoadedRy == ry; }),
        m_placements.end());
}

void SroWorldSession::UpdateCameraRegion(int& inOutRx, int& inOutRy, Vector3& cameraPos) {
    if (!m_isOpen) return;

    int newRx = inOutRx;
    int newRy = inOutRy;
    float shiftX = 0.0f;
    float shiftZ = 0.0f;
    constexpr float kBoundaryThreshold = 100.0f;

    if (cameraPos.x < -kBoundaryThreshold) {
        newRy -= 1;
        shiftX = REGION_SIZE;
    } else if (cameraPos.x > REGION_SIZE + kBoundaryThreshold) {
        newRy += 1;
        shiftX = -REGION_SIZE;
    }

    if (cameraPos.z < -kBoundaryThreshold) {
        newRx -= 1;
        shiftZ = REGION_SIZE;
    } else if (cameraPos.z > REGION_SIZE + kBoundaryThreshold) {
        newRx += 1;
        shiftZ = -REGION_SIZE;
    }

    if (newRx != inOutRx || newRy != inOutRy) {
        LoadRegion(newRx, newRy);
        cameraPos.x += shiftX;
        cameraPos.z += shiftZ;
        inOutRx = newRx;
        inOutRy = newRy;
        Logger::Instance().Info("Streamed to region " + std::to_string(newRx) + "," + std::to_string(newRy));
    }
}

void SroWorldSession::CollectPlacements(std::vector<PlacementVM>& out) const {
    out = m_placements;
}

void SroWorldSession::BuildScenePlacements(const std::vector<PlacementVM>& source,
    const std::function<bool(int16_t)>& isSelected,
    std::vector<ScenePlacement>& out) const {

    out.clear();
    out.reserve(source.size());

    for (const auto& p : source) {
        RegionId regionId(p.Object.RegionID);
        int objRx = regionId.Rx();
        int objRy = regionId.Ry();
        if (objRx == 0 && objRy == 0) {
            objRx = p.LoadedRx;
            objRy = p.LoadedRy;
        }

        if (std::abs(objRx - m_centerRx) > LoadRadius() || std::abs(objRy - m_centerRy) > LoadRadius()) {
            continue;
        }

        ScenePlacement sp;
        sp.localPosition = Vector3(p.Object.PosX, p.Object.PosY, p.Object.PosZ);
        sp.yaw = p.Object.Yaw;
        sp.bsrPath = p.BsrPath;
        sp.isSelected = isSelected && isSelected(p.Object.UID);
        sp.regionX = objRx;
        sp.regionY = objRy;
        sp.lod = p.Lod;
        sp.objectUid = p.Object.UID;
        sp.objId = p.Object.ObjID;
        out.push_back(sp);
    }
}

bool SroWorldSession::SavePlacements() { return m_regionManager && m_regionManager->SavePlacements(); }
bool SroWorldSession::SaveTerrain() { return m_regionManager && m_regionManager->SaveTerrain(); }
bool SroWorldSession::SaveNavmesh() { return m_regionManager && m_regionManager->SaveNavmesh(); }

void SroWorldSession::EnsurePlacementCollision(PlacementVM& vm) {
    if (vm.Collision.NavMesh || vm.BsrPath.empty()) return;
    if (auto* rm = m_renderer.GetRenderManager()) {
        if (auto* mesh = rm->GetMeshRenderer()) {
            auto* bsr = mesh->GetBsrResource(vm.BsrPath);
            if (!bsr) return;
            if (bsr->CollisionPath.size() || bsr->RequireCollisionMatrix ||
                (bsr->CollisionBox0[0] != 0.0f || bsr->CollisionBox0[3] != 0.0f)) {
                vm.Collision.HasAuthoredBox = true;
                vm.Collision.Box0 = bsr->CollisionBox0;
                vm.Collision.Box1 = bsr->CollisionBox1;
                vm.Collision.RequireCollisionMatrix = bsr->RequireCollisionMatrix;
                vm.Collision.CollisionMatrix = bsr->CollisionMatrix;
            }
            std::string resolvedPath;
            if (const BmsNavMesh* nav = mesh->LoadCollisionNavMesh(vm.BsrPath, &resolvedPath)) {
                vm.Collision.NavMesh = nav;
                vm.Collision.CollisionMeshPath = resolvedPath;
            }
        }
    }
}

void SroWorldSession::EagerLoadRegionPlacementCollision(int rx, int ry, std::vector<PlacementVM>& placements) {
    for (auto& p : placements) {
        if (p.LoadedRx == rx && p.LoadedRy == ry)
            EnsurePlacementCollision(p);
    }
}

void SroWorldSession::EnsurePlacementModel(PlacementVM& vm) {
    if (vm.BsrPath.empty()) return;
    if (auto* rm = m_renderer.GetRenderManager()) {
        if (auto* mesh = rm->GetMeshRenderer())
            mesh->PreloadModel(vm.BsrPath);
    }
}

} // namespace sro
