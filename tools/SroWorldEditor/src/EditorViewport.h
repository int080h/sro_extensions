#pragma once
#include "EditorContext.h"
#include "RendererD3D9.h"
#include "EditorCamera.h"
#include "ViewportFramebuffer.h"
#include "PlacementVM.h"
#include "runtime/SroWorldSession.h"
#include <memory>
#include <vector>
#include <string>

class RegionManager;

class EditorViewport {
public:
    void Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();
    void OnDeviceLost();
    void OnResize(float width, float height);
    void Update(EditorContext& ctx, float dt, bool focused, bool rmbDown, float mouseDx, float mouseDy, const bool keys[256]);
    void RenderToTexture(EditorContext& ctx, float w, float h);

    bool TryLoadSroClient(const std::wstring& clientPath, int rx, int ry,
        const Vector3* restorePosition = nullptr,
        float restoreYaw = 0.0f, float restorePitch = 0.0f);
    void SetSroRegion(int rx, int ry);

    EditorCamera& GetCamera() { return m_camera; }
    sro::SroWorldSession* GetSroSession() { return m_sroSession.get(); }
    RegionManager* GetRegionManager();
    LPDIRECT3DTEXTURE9 GetDisplayTexture() const;

private:
    void RenderSampleScene(EditorContext& ctx, float w, float h);
    void RenderSroScene(EditorContext& ctx, float w, float h);
    void DrawGrid(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj, float offsetX, float offsetZ);
    void DrawRegionBounds(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj, int rx, int ry, int centerRx, int centerRy);
    void DrawAxisGizmo(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj);
    void DrawPlaceholderObjects(EditorContext& ctx, LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj);
    void HandleViewportPick(EditorContext& ctx, bool focused, float vpW, float vpH);

    struct ViewportOverlayFlags {
        bool drawZones = false;
        bool drawSpawns = false;
        bool drawNpcs = false;
        bool drawTeleports = false;
        bool drawRegionDebug = false;
    };

    ViewportOverlayFlags ApplyViewportMode(const ViewportSettings& settings, sro::SceneRenderer& renderer);
    void DrawEditorOverlays(EditorContext& ctx, LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj,
                            const ViewportOverlayFlags& flags, float regionOffsetX, float regionOffsetZ,
                            int centerRx, int centerRy, int loadRadius);
    void DrawWireBox(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj,
                     float ox, float oy, float oz, float hx, float hy, float hz, D3DCOLOR color);
    void DrawMarkerCross(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj,
                         float x, float y, float z, float size, D3DCOLOR color);

    LPDIRECT3DDEVICE9 m_device = nullptr;
    ViewportFramebuffer m_framebuffer;
    EditorCamera m_camera;
    bool m_mouseCaptured = false;
    bool m_sroInitialized = false;
    std::wstring m_clientPath;
    std::unique_ptr<sro::SroWorldSession> m_sroSession;
    std::vector<PlacementVM> m_placements;
    int m_sroRx = sro::DEFAULT_REGION_RX;
    int m_sroRy = sro::DEFAULT_REGION_RY;
};

void SyncPlacementGlobalVector(int16_t uid, int rx, int ry, const Vector3& pos, float yaw, bool isDelete, const sro::formats::MapObject* spawnObj = nullptr, const std::string& bsrPath = "");
