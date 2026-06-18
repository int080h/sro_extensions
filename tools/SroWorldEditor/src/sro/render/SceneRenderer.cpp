#include "SceneRenderer.h"

namespace sro {

void SceneRenderer::Initialize(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath) {
    m_device = device;
    m_renderManager = std::make_unique<RenderManager>();
    m_renderManager->Initialize(device, clientPath);
}

void SceneRenderer::Shutdown() {
    if (m_renderManager) {
        m_renderManager->Cleanup();
        m_renderManager.reset();
    }
    m_device = nullptr;
}

void SceneRenderer::NewFrame() {
    if (m_renderManager) m_renderManager->NewFrame();
}

void SceneRenderer::DrawSky(float width, float height) {
    if (m_renderManager) m_renderManager->DrawSky(width, height);
}

void SceneRenderer::DrawScene(const Matrix4& view, const Matrix4& proj, const Vector3& cameraPos,
                              int centerRx, int centerRy, int loadRadius, const CameraFrustum& frustum,
                              const std::vector<ScenePlacement>& placements) {
    if (!m_renderManager) return;

    m_renderManager->ShowTerrain = ShowTerrain;
    m_renderManager->ShowObjects = ShowObjects;
    m_renderManager->ShowWalkable = ShowWalkable;
    m_renderManager->ShowBlocked = ShowBlocked;
    m_renderManager->ShowInternalEdges = ShowInternalEdges;
    m_renderManager->ShowGlobalEdges = ShowGlobalEdges;
    m_renderManager->ShowCells = ShowCells;
    m_renderManager->ShowEventDecors = ShowEventDecors;
    m_renderManager->ShowParticles = ShowParticles;
    m_renderManager->WireframeMode = WireframeMode;

    std::vector<RenderPlacementVM> vms;
    vms.reserve(placements.size());
    for (const auto& p : placements) {
        RenderPlacementVM vm;
        vm.Position = p.localPosition;
        vm.Yaw = p.yaw;
        vm.BsrPath = p.bsrPath;
        vm.IsSelected = p.isSelected;
        vm.RegionX = p.regionX;
        vm.RegionY = p.regionY;
        vm.Lod = p.lod;
        vm.ObjectUID = p.objectUid;
        vm.ObjID = p.objId;
        vms.push_back(vm);
    }

    m_renderManager->Draw3DScene(view, proj, cameraPos, centerRx, centerRy, loadRadius, frustum, vms);
}

} // namespace sro
