#pragma once
#include "EditorContext.h"
#include "RendererD3D9.h"
#include "EditorCamera.h"
#include "ViewportFramebuffer.h"
#include "PlacementVM.h"
#include "runtime/SroWorldSession.h"
#include "nav/NavMeshSession.h"
#include "core/ClientLoadProgress.h"
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

    struct ClientLoadRequest {
        std::wstring clientPath;
        int rx = 97;
        int ry = 167;
        Vector3 restorePosition{};
        float restoreYaw = -90.f;
        float restorePitch = -30.f;
        bool useRestoreCamera = false;
    };

    void BeginClientLoad(const ClientLoadRequest& request);
    bool TickClientLoad(ClientLoadProgress& progress);
    bool IsClientLoadActive() const { return m_clientLoadActive; }
    const ClientLoadProgress& GetClientLoadProgress() const { return m_clientLoadProgress; }

    void SetSroRegion(int rx, int ry);

    EditorCamera& GetCamera() { return m_camera; }
    sro::SroWorldSession* GetSroSession() { return m_sroSession.get(); }
    RegionManager* GetRegionManager();
    LPDIRECT3DTEXTURE9 GetDisplayTexture() const;
    LPDIRECT3DDEVICE9 GetDevice() const { return m_device; }

private:
    void RenderSampleScene(EditorContext& ctx, float w, float h);
    void RenderSroScene(EditorContext& ctx, float w, float h);
    void DrawGrid(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj, float offsetX, float offsetZ);
    void DrawRegionBounds(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj, int rx, int ry, int centerRx, int centerRy);
    void DrawAxisGizmo(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj);
    void DrawPlaceholderObjects(EditorContext& ctx, LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj);
    void SyncClientContext(EditorContext& ctx);
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
    void DrawNpcMeshes(EditorContext& ctx, const Matrix4& view, const Matrix4& proj, float regionOffsetX, float regionOffsetZ);
    bool RaycastNpcs(EditorContext& ctx, const Vector3& rayPos, const Vector3& rayDir, float regionOffsetX, float regionOffsetZ, float& outDist);
    void HandleNpcPlacementClick(EditorContext& ctx, const Vector3& worldPos);

    LPDIRECT3DDEVICE9 m_device = nullptr;
    ViewportFramebuffer m_framebuffer;
    EditorCamera m_camera;
    bool m_mouseCaptured = false;
    bool m_sroInitialized = false;
    std::wstring m_clientPath;
    std::unique_ptr<sro::SroWorldSession> m_sroSession;
    std::unique_ptr<sro::nav::NavMeshSession> m_navMeshSession;
    std::vector<PlacementVM> m_placements;
    std::vector<sro::ScenePlacement> m_scenePlacements;
    int m_sroRx = sro::DEFAULT_REGION_RX;
    int m_sroRy = sro::DEFAULT_REGION_RY;

    bool m_clientLoadActive = false;
    ClientLoadProgress m_clientLoadProgress{};
    ClientLoadRequest m_clientLoadRequest{};
    int m_clientLoadStep = 0;
    int m_clientLoadTotalSteps = 1;
};

void SyncPlacementGlobalVector(int16_t uid, int rx, int ry, const Vector3& pos, float yaw, bool isDelete,
    const sro::formats::MapObject* spawnObj = nullptr, const std::string& bsrPath = "",
    int blockZ = 0, int blockX = 0, int lod = 0);
