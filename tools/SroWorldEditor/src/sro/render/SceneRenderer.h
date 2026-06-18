#pragma once
#include <d3d9.h>
#include <memory>
#include <string>
#include <vector>
#include "Rendering/RenderManager.h"
#include "Rendering/Camera.h"
#include "core/MathTypes.h"

namespace sro {

struct ScenePlacement {
    Vector3 localPosition;
    float yaw = 0;
    std::string bsrPath;
    bool isSelected = false;
    int regionX = 0;
    int regionY = 0;
    int lod = 0;
    int16_t objectUid = 0;
    uint32_t objId = 0;
};

class SceneRenderer {
public:
    void Initialize(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath);
    void Shutdown();
    void NewFrame();

    void DrawSky(float width, float height);
    void DrawScene(const Matrix4& view, const Matrix4& proj, const Vector3& cameraPos,
                   int centerRx, int centerRy, int loadRadius, const CameraFrustum& frustum,
                   const std::vector<ScenePlacement>& placements);

    RenderManager* GetRenderManager() { return m_renderManager.get(); }
    const RenderManager* GetRenderManager() const { return m_renderManager.get(); }

    bool ShowTerrain = true;
    bool ShowObjects = true;
    bool ShowWalkable = true;
    bool ShowBlocked = true;
    bool ShowInternalEdges = false;
    bool ShowGlobalEdges = false;
    bool ShowCells = false;
    bool ShowEventDecors = false;
    bool ShowParticles = true;
    bool WireframeMode = false;

private:
    LPDIRECT3DDEVICE9 m_device = nullptr;
    std::unique_ptr<RenderManager> m_renderManager;
};

} // namespace sro
