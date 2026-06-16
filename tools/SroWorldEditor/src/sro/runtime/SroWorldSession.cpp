#include "SroWorldSession.h"
#include "cache/ClientDataCache.h"
#include "core/FileSystem.h"
#include "core/RegionId.h"
#include <algorithm>

namespace sro {

bool SroWorldSession::OpenClient(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath) {
    if (!device || !FileExists(clientPath)) return false;

    Shutdown();
    m_clientPath = clientPath;
    m_cache.SetClientPath(clientPath);
    ClientDataCache::Instance().SetClientPath(clientPath);
    m_assets.Initialize(clientPath);
    m_renderer.Initialize(device, clientPath);
    m_regionManager = std::make_unique<RegionManager>();
    m_regionManager->Initialize(clientPath, m_renderer.GetRenderManager());
    SetLoadRadius(1);

    LoadMapInfo();
    if (auto* rm = m_renderer.GetRenderManager()) {
        if (auto* wmc = rm->GetWorldMapControl()) {
            wmc->ActiveRegions = m_mapProject.ActiveRegions;
            wmc->ScanRegionFiles(clientPath);
            wmc->MapStyle = 0;
            wmc->CurrentRx = m_centerRx;
            wmc->CurrentRy = m_centerRy;
        }
    }

    m_isOpen = true;
    Logger::Instance().Info("SRO client opened: " + ToNarrow(clientPath));
    return true;
}

void SroWorldSession::Shutdown() {
    m_placements.clear();
    m_gpuPlacementRegions.clear();
    m_regionManager.reset();
    m_renderer.Shutdown();
    m_isOpen = false;
}

void SroWorldSession::LoadMapInfo() {
    auto path = MapInfoPath(m_clientPath);
    if (FileExists(path)) {
        if (MapProjectParser::Read(path, m_mapProject).ok) {
            Logger::Instance().Info("Loaded mapinfo.mfo: " + std::to_string(m_mapProject.ActiveRegions.size()) + " active regions");
        }
    } else {
        Logger::Instance().Warning("mapinfo.mfo not found");
    }
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

    const int lodCount = placements->LodCount();
    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            for (int lod = 0; lod < lodCount; ++lod) {
                int idx = 0;
                for (const auto& obj : placements->Blocks[z][x][lod]) {
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

} // namespace sro
