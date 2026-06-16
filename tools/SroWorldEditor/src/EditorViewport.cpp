#include "EditorViewport.h"
#include "PlacementVM.h"
#include "core/Logger.h"
#include "core/FileSystem.h"
#include "core/SceneSpace.h"
#include "Rendering/Camera.h"
#include "Rendering/RenderManager.h"
#include "render/SceneRenderer.h"
#include "runtime/RegionManager.h"
#include <imgui.h>
#include <cmath>
#include <exception>

static std::vector<PlacementVM> g_placementBridge;

void SyncPlacementGlobalVector(int16_t uid, int rx, int ry, const Vector3& pos, float yaw, bool isDelete, const sro::formats::MapObject* spawnObj, const std::string& bsrPath) {
    if (isDelete) {
        g_placementBridge.erase(std::remove_if(g_placementBridge.begin(), g_placementBridge.end(), [&](const PlacementVM& p) {
            return p.Object.UID == uid && p.LoadedRx == rx && p.LoadedRy == ry;
        }), g_placementBridge.end());
    } else if (spawnObj) {
        PlacementVM vm;
        vm.Object = *spawnObj;
        vm.BsrPath = bsrPath;
        vm.LoadedRx = rx;
        vm.LoadedRy = ry;
        g_placementBridge.push_back(vm);
    } else {
        for (auto& p : g_placementBridge) {
            if (p.Object.UID == uid && p.LoadedRx == rx && p.LoadedRy == ry) {
                p.Object.PosX = pos.x;
                p.Object.PosY = pos.y;
                p.Object.PosZ = pos.z;
                p.Object.Yaw = yaw;
                break;
            }
        }
    }
}

struct D3DVertex { float x, y, z; D3DCOLOR color; };
#define D3DFVF_CUSTOM (D3DFVF_XYZ | D3DFVF_DIFFUSE)

RegionManager* EditorViewport::GetRegionManager() {
    return m_sroSession ? m_sroSession->LegacyRegionManager() : nullptr;
}

void EditorViewport::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;
    m_framebuffer.Initialize(device);
    m_camera = EditorCamera(m_camera.Position, 1.77f);
}

void EditorViewport::Shutdown() {
    m_framebuffer.Shutdown();
    m_sroSession.reset();
    m_sroInitialized = false;
}

void EditorViewport::OnDeviceLost() {
    m_framebuffer.OnDeviceLost();
}

void EditorViewport::OnResize(float width, float height) {
    if (width > 0 && height > 0) {
        m_camera.AspectRatio = width / height;
    }
}

bool EditorViewport::TryLoadSroClient(const std::wstring& clientPath, int rx, int ry,
    const Vector3* restorePosition, float restoreYaw, float restorePitch) {
    if (!m_device || !FileExists(clientPath)) return false;
    try {
        m_clientPath = clientPath;
        m_sroSession = std::make_unique<sro::SroWorldSession>();
        if (!m_sroSession->OpenClient(m_device, clientPath)) {
            m_sroSession.reset();
            return false;
        }
        m_sroRx = rx;
        m_sroRy = ry;
        if (!m_sroSession->LoadRegion(rx, ry)) {
            m_sroSession.reset();
            m_sroInitialized = false;
            return false;
        }
        m_sroSession->CollectPlacements(m_placements);
        g_placementBridge = m_placements;
        m_camera.SetLoadRadius(m_sroSession->LoadRadius());
        if (restorePosition) {
            m_camera.Position = *restorePosition;
            m_camera.Yaw = restoreYaw;
            m_camera.Pitch = restorePitch;
        } else {
            m_camera.Position = Vector3(960.0f, 150.0f, 960.0f);
        }
        m_sroInitialized = true;
        Logger::Instance().Info("Loaded SRO client region " + std::to_string(rx) + "," + std::to_string(ry));
        return true;
    } catch (const std::exception& ex) {
        Logger::Instance().Error(std::string("TryLoadSroClient failed: ") + ex.what());
    } catch (...) {
        Logger::Instance().Error("TryLoadSroClient failed: unknown exception");
    }
    m_sroSession.reset();
    m_sroInitialized = false;
    return false;
}

void EditorViewport::SetSroRegion(int rx, int ry) {
    if (!m_sroSession) return;
    m_sroRx = rx;
    m_sroRy = ry;
    m_sroSession->LoadRegion(rx, ry);
    m_sroSession->CollectPlacements(m_placements);
    g_placementBridge = m_placements;
}

void EditorViewport::HandleViewportPick(EditorContext& ctx, bool focused, float vpW, float vpH) {
    if (!focused || !m_sroSession || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;
    if (ImGui::GetIO().WantCaptureMouse || vpW < 1 || vpH < 1) return;

    auto* rm = m_sroSession->Renderer().GetRenderManager();
    if (!rm) return;
    rm->ShowEventDecors = ctx.viewport.showEventDecors;

    ImVec2 m = ImGui::GetMousePos();
    float ndcX = (2.0f * m.x) / vpW - 1.0f;
    float ndcY = 1.0f - (2.0f * m.y) / vpH;

    float fovRad = m_camera.Fov * (3.14159265f / 180.0f);
    float halfHeight = tanf(fovRad / 2.0f);
    float halfWidth = halfHeight * m_camera.AspectRatio;
    Vector3 rayDir = m_camera.Front + (m_camera.Right * (ndcX * halfWidth)) + (m_camera.Up * (ndcY * halfHeight));
    rayDir = Vec3Normalize(rayDir);
    Vector3 rayPos = m_camera.Position;

    std::vector<RenderPlacementVM> vms;
    for (const auto& p : m_placements) {
        int objRx = (p.Object.RegionID >> 8) & 0xFF;
        int objRy = p.Object.RegionID & 0xFF;
        if (objRx == 0 && objRy == 0) { objRx = p.LoadedRx; objRy = p.LoadedRy; }

        RenderPlacementVM vm;
        vm.Position = Vector3(p.Object.PosX, p.Object.PosY, p.Object.PosZ);
        vm.Yaw = p.Object.Yaw;
        vm.BsrPath = p.BsrPath;
        vm.IsSelected = ctx.selection && ctx.selection->kind == EntityKind::MapPlacement
            && ctx.selection->id == std::to_string(p.Object.UID);
        vm.RegionX = objRx;
        vm.RegionY = objRy;
        vm.Lod = p.Lod;
        vm.ObjectUID = p.Object.UID;
        vm.ObjID = p.Object.ObjID;
        vms.push_back(vm);
    }

    float hitDist = 0;
    int hitIdx = rm->RaycastObjects(rayPos, rayDir, vms, m_sroRx, m_sroRy, hitDist);
    if (hitIdx >= 0 && hitIdx < static_cast<int>(m_placements.size())) {
        const auto& p = m_placements[hitIdx];
        ctx.Select({EntityKind::MapPlacement,
            EditorContext::EncodeRegionId(p.LoadedRx, p.LoadedRy),
            std::to_string(p.Object.UID)});
    }
}

void EditorViewport::Update(EditorContext& ctx, float dt, bool focused, bool rmbDown, float mouseDx, float mouseDy, const bool keys[256]) {
    if (!focused) {
        m_mouseCaptured = false;
        return;
    }
    if (rmbDown) {
        if (!m_mouseCaptured) m_mouseCaptured = true;
        m_camera.ProcessMouseMovement(mouseDx, mouseDy);
    } else {
        m_mouseCaptured = false;
    }
    float velScale = ctx.viewport.cameraSpeed / 250.0f;
    if (keys['W']) m_camera.ProcessKeyboard(1, dt * velScale, keys[VK_SHIFT]);
    if (keys['S']) m_camera.ProcessKeyboard(2, dt * velScale, keys[VK_SHIFT]);
    if (keys['A']) m_camera.ProcessKeyboard(3, dt * velScale, keys[VK_SHIFT]);
    if (keys['D']) m_camera.ProcessKeyboard(4, dt * velScale, keys[VK_SHIFT]);
    if (keys['E']) m_camera.ProcessKeyboard(5, dt * velScale, keys[VK_SHIFT]);
    if (keys['Q']) m_camera.ProcessKeyboard(6, dt * velScale, keys[VK_SHIFT]);

    ctx.project.cameraPosition = m_camera.Position;
    ctx.project.cameraYaw = m_camera.Yaw;
    ctx.project.cameraPitch = m_camera.Pitch;

    if (ctx.sroClientLoaded && m_sroSession) {
        const int prevRx = m_sroRx;
        const int prevRy = m_sroRy;
        m_sroSession->UpdateCameraRegion(m_sroRx, m_sroRy, m_camera.Position);
        if (prevRx != m_sroRx || prevRy != m_sroRy) {
            m_sroSession->CollectPlacements(m_placements);
            g_placementBridge = m_placements;
        }
        ctx.sroRegionRx = m_sroRx;
        ctx.sroRegionRy = m_sroRy;
        ctx.sroPlacements = &m_placements;
        ctx.sroRegionManager = m_sroSession->LegacyRegionManager();
        if (auto* rm = m_sroSession->Renderer().GetRenderManager()) {
            ctx.sroRenderManager = rm;
            ctx.sroMeshRenderer = rm->GetMeshRenderer();
        }
        ctx.mouseWorldPos = m_camera.Position;
        HandleViewportPick(ctx, focused, (float)m_framebuffer.Width(), (float)m_framebuffer.Height());
    } else {
        ctx.sroPlacements = nullptr;
        ctx.sroRegionManager = nullptr;
        ctx.sroRenderManager = nullptr;
        ctx.sroMeshRenderer = nullptr;
        float regionOffset = (ctx.activeRegionId - 25000) * 1920.0f;
        ctx.mouseWorldPos = m_camera.Position + Vector3(regionOffset, 0, 0);
    }
}

void EditorViewport::DrawGrid(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj, float offsetX, float offsetZ) {
    std::vector<D3DVertex> verts;
    const float extent = 1920.0f;
    const float step = 96.0f;
    auto addLine = [&](float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR c) {
        verts.push_back({x1, y1, z1, c});
        verts.push_back({x2, y2, z2, c});
    };
    D3DCOLOR gridCol = D3DCOLOR_RGBA(60, 65, 75, 255);
    for (float i = 0; i <= extent; i += step) {
        addLine(offsetX + i, 0, offsetZ, offsetX + i, 0, offsetZ + extent, gridCol);
        addLine(offsetX, 0, offsetZ + i, offsetX + extent, 0, offsetZ + i, gridCol);
    }
    device->SetFVF(D3DFVF_CUSTOM);
    device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);
    device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTexture(0, nullptr);
    if (!verts.empty()) {
        device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)verts.size() / 2, verts.data(), sizeof(D3DVertex));
    }
}

void EditorViewport::DrawRegionBounds(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj, int rx, int ry, int centerRx, int centerRy) {
    float offsetX = sro::SceneSpace::SceneOffsetX(ry, centerRy);
    float offsetZ = sro::SceneSpace::SceneOffsetZ(rx, centerRx);
    std::vector<D3DVertex> verts;
    float y = 5.0f;
    float s = 1920.0f;
    D3DCOLOR c = D3DCOLOR_RGBA(255, 200, 50, 255);
    auto line = [&](float x1, float z1, float x2, float z2) {
        verts.push_back({offsetX + x1, y, offsetZ + z1, c});
        verts.push_back({offsetX + x2, y, offsetZ + z2, c});
    };
    line(0, 0, s, 0); line(s, 0, s, s); line(s, s, 0, s); line(0, s, 0, 0);
    device->SetFVF(D3DFVF_CUSTOM);
    device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);
    device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)verts.size() / 2, verts.data(), sizeof(D3DVertex));
}

void EditorViewport::DrawAxisGizmo(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj) {
    std::vector<D3DVertex> verts = {
        {0, 0, 0, D3DCOLOR_RGBA(255, 60, 60, 255)}, {200, 0, 0, D3DCOLOR_RGBA(255, 60, 60, 255)},
        {0, 0, 0, D3DCOLOR_RGBA(60, 255, 60, 255)}, {0, 200, 0, D3DCOLOR_RGBA(60, 255, 60, 255)},
        {0, 0, 0, D3DCOLOR_RGBA(60, 120, 255, 255)}, {0, 0, 200, D3DCOLOR_RGBA(60, 120, 255, 255)},
    };
    device->SetFVF(D3DFVF_CUSTOM);
    device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);
    device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->DrawPrimitiveUP(D3DPT_LINELIST, 3, verts.data(), sizeof(D3DVertex));
}

void EditorViewport::DrawPlaceholderObjects(EditorContext& ctx, LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj) {
    if (auto* region = ctx.world.FindRegion(ctx.activeRegionId)) {
        for (const auto& obj : region->objects) {
            if (!obj.visible) continue;
            float ox = obj.transform.position.x;
            float oy = obj.transform.position.y + 25.0f;
            float oz = obj.transform.position.z;
            float hs = 25.0f;
            bool selected = ctx.selection && ctx.selection->id == obj.id;
            D3DCOLOR col = selected ? D3DCOLOR_RGBA(255, 220, 80, 255) : D3DCOLOR_RGBA(120, 160, 220, 255);
            std::vector<D3DVertex> box = {
                {ox-hs,oy-hs,oz-hs,col},{ox+hs,oy-hs,oz-hs,col},{ox+hs,oy-hs,oz-hs,col},{ox+hs,oy-hs,oz+hs,col},
                {ox+hs,oy-hs,oz+hs,col},{ox-hs,oy-hs,oz+hs,col},{ox-hs,oy-hs,oz+hs,col},{ox-hs,oy-hs,oz-hs,col},
                {ox-hs,oy+hs,oz-hs,col},{ox+hs,oy+hs,oz-hs,col},{ox+hs,oy+hs,oz-hs,col},{ox+hs,oy+hs,oz+hs,col},
                {ox+hs,oy+hs,oz+hs,col},{ox-hs,oy+hs,oz+hs,col},{ox-hs,oy+hs,oz+hs,col},{ox-hs,oy+hs,oz-hs,col},
                {ox-hs,oy-hs,oz-hs,col},{ox-hs,oy+hs,oz-hs,col},{ox+hs,oy-hs,oz-hs,col},{ox+hs,oy+hs,oz-hs,col},
                {ox+hs,oy-hs,oz+hs,col},{ox+hs,oy+hs,oz+hs,col},{ox-hs,oy-hs,oz+hs,col},{ox-hs,oy+hs,oz+hs,col},
            };
            device->SetFVF(D3DFVF_CUSTOM);
            Matrix4 identity = Matrix4::Identity();
            device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&identity);
            device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);
            device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);
            device->SetRenderState(D3DRS_LIGHTING, FALSE);
            device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)box.size() / 2, box.data(), sizeof(D3DVertex));
        }
    }
}

void EditorViewport::DrawWireBox(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj,
                                 float ox, float oy, float oz, float hx, float hy, float hz, D3DCOLOR color) {
    std::vector<D3DVertex> box = {
        {ox-hx,oy-hy,oz-hz,color},{ox+hx,oy-hy,oz-hz,color},{ox+hx,oy-hy,oz-hz,color},{ox+hx,oy-hy,oz+hz,color},
        {ox+hx,oy-hy,oz+hz,color},{ox-hx,oy-hy,oz+hz,color},{ox-hx,oy-hy,oz+hz,color},{ox-hx,oy-hy,oz-hz,color},
        {ox-hx,oy+hy,oz-hz,color},{ox+hx,oy+hy,oz-hz,color},{ox+hx,oy+hy,oz-hz,color},{ox+hx,oy+hy,oz+hz,color},
        {ox+hx,oy+hy,oz+hz,color},{ox-hx,oy+hy,oz+hz,color},{ox-hx,oy+hy,oz+hz,color},{ox-hx,oy+hy,oz-hz,color},
        {ox-hx,oy-hy,oz-hz,color},{ox-hx,oy+hy,oz-hz,color},{ox+hx,oy-hy,oz-hz,color},{ox+hx,oy+hy,oz-hz,color},
        {ox+hx,oy-hy,oz+hz,color},{ox+hx,oy+hy,oz+hz,color},{ox-hx,oy-hy,oz+hz,color},{ox-hx,oy+hy,oz+hz,color},
    };
    device->SetFVF(D3DFVF_CUSTOM);
    Matrix4 identity = Matrix4::Identity();
    device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&identity);
    device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);
    device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTexture(0, nullptr);
    device->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)box.size() / 2, box.data(), sizeof(D3DVertex));
}

void EditorViewport::DrawMarkerCross(LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj,
                                     float x, float y, float z, float size, D3DCOLOR color) {
    std::vector<D3DVertex> verts = {
        {x - size, y, z, color}, {x + size, y, z, color},
        {x, y, z - size, color}, {x, y, z + size, color},
        {x, y, z, color}, {x, y + size * 2.0f, z, color},
    };
    device->SetFVF(D3DFVF_CUSTOM);
    Matrix4 identity = Matrix4::Identity();
    device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&identity);
    device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);
    device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTexture(0, nullptr);
    device->DrawPrimitiveUP(D3DPT_LINELIST, 3, verts.data(), sizeof(D3DVertex));
}

EditorViewport::ViewportOverlayFlags EditorViewport::ApplyViewportMode(const ViewportSettings& settings, sro::SceneRenderer& renderer) {
    ViewportOverlayFlags flags{};
    switch (settings.viewMode) {
    case ViewportMode::Lit:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = settings.showCollision;
        renderer.ShowBlocked = settings.showCollision;
        renderer.ShowEdges = settings.showCollision;
        renderer.WireframeMode = false;
        break;
    case ViewportMode::Wireframe:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowEdges = false;
        renderer.WireframeMode = true;
        break;
    case ViewportMode::Collision:
        renderer.ShowTerrain = false;
        renderer.ShowObjects = false;
        renderer.ShowWalkable = true;
        renderer.ShowBlocked = true;
        renderer.ShowEdges = true;
        renderer.WireframeMode = false;
        break;
    case ViewportMode::ZoneOverlay:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowEdges = false;
        renderer.WireframeMode = false;
        flags.drawZones = true;
        break;
    case ViewportMode::SpawnOverlay:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowEdges = false;
        renderer.WireframeMode = false;
        flags.drawSpawns = settings.showSpawns;
        flags.drawNpcs = settings.showNpcs;
        flags.drawTeleports = settings.showTeleports;
        break;
    case ViewportMode::RegionDebug:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowEdges = false;
        renderer.WireframeMode = false;
        flags.drawRegionDebug = true;
        break;
    }
    return flags;
}

void EditorViewport::DrawEditorOverlays(EditorContext& ctx, LPDIRECT3DDEVICE9 device, const Matrix4& view, const Matrix4& proj,
                                      const ViewportOverlayFlags& flags, float regionOffsetX, float regionOffsetZ,
                                      int centerRx, int centerRy, int loadRadius) {
    if (!flags.drawZones && !flags.drawSpawns && !flags.drawNpcs && !flags.drawTeleports && !flags.drawRegionDebug) {
        return;
    }

    for (const auto& region : ctx.world.regions) {
        float rOffset = (region.id - 25000) * 1920.0f;
        float baseX = regionOffsetX + rOffset;
        float baseZ = regionOffsetZ;

        if (flags.drawZones) {
            for (const auto& zone : region.zones) {
                float cx = baseX + (zone.boundsMin.x + zone.boundsMax.x) * 0.5f;
                float cy = baseZ + (zone.boundsMin.z + zone.boundsMax.z) * 0.5f;
                float cz = (zone.boundsMin.y + zone.boundsMax.y) * 0.5f;
                float hx = (zone.boundsMax.x - zone.boundsMin.x) * 0.5f;
                float hy = (zone.boundsMax.y - zone.boundsMin.y) * 0.5f;
                float hz = (zone.boundsMax.z - zone.boundsMin.z) * 0.5f;
                D3DCOLOR col = zone.pvpEnabled ? D3DCOLOR_RGBA(255, 80, 80, 255)
                    : (zone.safeZone ? D3DCOLOR_RGBA(80, 200, 120, 255) : D3DCOLOR_RGBA(120, 160, 255, 255));
                DrawWireBox(device, view, proj, cx, cz, cy, hx, hy, hz, col);
            }
        }

        if (flags.drawSpawns) {
            for (const auto& sp : region.spawns) {
                DrawMarkerCross(device, view, proj, baseX + sp.position.x, sp.position.y, baseZ + sp.position.z, 15.0f,
                                D3DCOLOR_RGBA(255, 120, 60, 255));
            }
        }
        if (flags.drawNpcs) {
            for (const auto& npc : region.npcs) {
                DrawMarkerCross(device, view, proj,
                    baseX + npc.transform.position.x, npc.transform.position.y, baseZ + npc.transform.position.z,
                    12.0f, D3DCOLOR_RGBA(80, 200, 255, 255));
            }
        }
        if (flags.drawTeleports) {
            for (const auto& tp : region.teleports) {
                DrawMarkerCross(device, view, proj,
                    baseX + tp.sourcePosition.x, tp.sourcePosition.y, baseZ + tp.sourcePosition.z,
                    12.0f, D3DCOLOR_RGBA(200, 120, 255, 255));
            }
        }
    }

    if (flags.drawRegionDebug) {
        int r = loadRadius > 0 ? loadRadius : 1;
        for (int drx = -r; drx <= r; ++drx) {
            for (int dry = -r; dry <= r; ++dry) {
                int rx = centerRx + drx;
                int ry = centerRy + dry;
                DrawRegionBounds(device, view, proj, rx, ry, centerRx, centerRy);
            }
        }
    }
}

void EditorViewport::RenderSampleScene(EditorContext& ctx, float w, float h) {
    if (!m_device || w < 1 || h < 1) return;
    m_camera.AspectRatio = w / h;
    Matrix4 view = m_camera.GetViewMatrix();
    Matrix4 proj = m_camera.GetProjectionMatrix();

    m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(30, 32, 38, 255), 1.0f, 0);

    bool wireframe = ctx.viewport.viewMode == ViewportMode::Wireframe;
    if (wireframe) {
        m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    }

    if (ctx.viewport.showGrid) DrawGrid(m_device, view, proj, 0, 0);
    if (ctx.viewport.showRegionBounds || ctx.viewport.viewMode == ViewportMode::RegionDebug) {
        DrawRegionBounds(m_device, view, proj, 0, 0, 0, 0);
    }
    DrawAxisGizmo(m_device, view, proj);
    DrawPlaceholderObjects(ctx, m_device, view, proj);

    ViewportOverlayFlags overlayFlags{};
    switch (ctx.viewport.viewMode) {
    case ViewportMode::ZoneOverlay: overlayFlags.drawZones = true; break;
    case ViewportMode::SpawnOverlay:
        overlayFlags.drawSpawns = ctx.viewport.showSpawns;
        overlayFlags.drawNpcs = ctx.viewport.showNpcs;
        overlayFlags.drawTeleports = ctx.viewport.showTeleports;
        break;
    case ViewportMode::RegionDebug: overlayFlags.drawRegionDebug = true; break;
    default: break;
    }
    float regionOffset = (ctx.activeRegionId - 25000) * 1920.0f;
    DrawEditorOverlays(ctx, m_device, view, proj, overlayFlags, regionOffset, 0.0f, 0, 0, 1);

    if (wireframe) {
        m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    }
}

void EditorViewport::RenderSroScene(EditorContext& ctx, float w, float h) {
    if (!m_device || !m_sroSession || w < 1 || h < 1) return;
    m_camera.AspectRatio = w / h;
    Matrix4 view = m_camera.GetViewMatrix();
    Matrix4 proj = m_camera.GetProjectionMatrix();
    Matrix4 vpMat = view * proj;
    CameraFrustum frustum = CameraFrustum::Extract(vpMat);

    m_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(20, 24, 32, 255), 1.0f, 0);

    auto& renderer = m_sroSession->Renderer();
    auto overlayFlags = ApplyViewportMode(ctx.viewport, renderer);
    renderer.ShowEventDecors = ctx.viewport.showEventDecors;
    renderer.NewFrame();
    renderer.DrawSky(w, h);

    std::vector<sro::ScenePlacement> scenePlacements;
    m_sroSession->BuildScenePlacements(m_placements,
        [&](int16_t uid) {
            return ctx.selection && ctx.selection->kind == EntityKind::MapPlacement
                && ctx.selection->id == std::to_string(uid);
        },
        scenePlacements);

    renderer.DrawScene(view, proj, m_camera.Position, m_sroRx, m_sroRy,
        m_sroSession->LoadRadius(), frustum, scenePlacements);

    if (ctx.viewport.showGrid) DrawGrid(m_device, view, proj, 0, 0);
    if (ctx.viewport.showRegionBounds || ctx.viewport.viewMode == ViewportMode::RegionDebug) {
        DrawRegionBounds(m_device, view, proj, m_sroRx, m_sroRy, m_sroRx, m_sroRy);
    }

    DrawEditorOverlays(ctx, m_device, view, proj, overlayFlags, 0.0f, 0.0f,
        m_sroRx, m_sroRy, m_sroSession->LoadRadius());
}

void EditorViewport::RenderToTexture(EditorContext& ctx, float w, float h) {
    if (!m_device || w < 1.0f || h < 1.0f) return;

    const UINT tw = (UINT)w;
    const UINT th = (UINT)h;
    if (!m_framebuffer.EnsureSize(tw, th)) return;
    if (!m_framebuffer.BeginRender()) return;

    if (ctx.sroClientLoaded && m_sroInitialized) {
        RenderSroScene(ctx, w, h);
    } else {
        RenderSampleScene(ctx, w, h);
    }

    m_framebuffer.EndRender();
}

LPDIRECT3DTEXTURE9 EditorViewport::GetDisplayTexture() const {
    return m_framebuffer.ColorTexture();
}
