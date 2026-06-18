#include "EditorViewport.h"
#include "PlacementVM.h"
#include "core/Logger.h"
#include "core/FileSystem.h"
#include "core/SceneSpace.h"
#include "index/TextDataCatalog.h"
#include "Rendering/Camera.h"
#include "Rendering/RenderManager.h"
#include "render/SceneRenderer.h"
#include "runtime/RegionManager.h"
#include "rendering/MeshRenderer.h"
#include "rendering/AiNavDataRenderer.h"
#include "core/EditorAction.h"
#include <imgui.h>
#include <cmath>
#include <exception>

static std::vector<PlacementVM>* g_activePlacements = nullptr;

void SyncPlacementGlobalVector(int16_t uid, int rx, int ry, const Vector3& pos, float yaw, bool isDelete,
    const sro::formats::MapObject* spawnObj, const std::string& bsrPath, int blockZ, int blockX, int lod) {
    if (!g_activePlacements) return;

    if (isDelete) {
        g_activePlacements->erase(std::remove_if(g_activePlacements->begin(), g_activePlacements->end(), [&](const PlacementVM& p) {
            return p.Object.UID == uid && p.LoadedRx == rx && p.LoadedRy == ry;
        }), g_activePlacements->end());
    } else if (spawnObj) {
        PlacementVM vm;
        vm.Object = *spawnObj;
        vm.BsrPath = bsrPath;
        vm.LoadedRx = rx;
        vm.LoadedRy = ry;
        vm.BlockZ = blockZ;
        vm.BlockX = blockX;
        vm.Lod = lod;
        vm.Index = 0;
        for (const auto& p : *g_activePlacements) {
            if (p.LoadedRx == rx && p.LoadedRy == ry && p.BlockZ == blockZ && p.BlockX == blockX && p.Lod == lod)
                vm.Index = (std::max)(vm.Index, p.Index + 1);
        }
        g_activePlacements->push_back(vm);
    } else {
        for (auto& p : *g_activePlacements) {
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
    m_navMeshSession.reset();
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
            m_navMeshSession.reset();
            return false;
        }
        m_sroRx = rx;
        m_sroRy = ry;
        if (!m_sroSession->LoadRegion(rx, ry)) {
            m_sroSession.reset();
            m_navMeshSession.reset();
            m_sroInitialized = false;
            return false;
        }
        m_sroSession->CollectPlacements(m_placements);
        g_activePlacements = &m_placements;
        m_camera.SetLoadRadius(m_sroSession->LoadRadius());
        if (restorePosition) {
            m_camera.Position = *restorePosition;
            m_camera.Yaw = restoreYaw;
            m_camera.Pitch = restorePitch;
        } else {
            m_camera.Position = Vector3(960.0f, 150.0f, 960.0f);
        }
        m_sroInitialized = true;
        m_navMeshSession = std::make_unique<sro::nav::NavMeshSession>();
        m_navMeshSession->SetClientPath(m_clientPath);
        m_navMeshSession->Scan();
        Logger::Instance().Info("Loaded SRO client region " + std::to_string(rx) + "," + std::to_string(ry));
        return true;
    } catch (const std::exception& ex) {
        Logger::Instance().Error(std::string("TryLoadSroClient failed: ") + ex.what());
    } catch (...) {
        Logger::Instance().Error("TryLoadSroClient failed: unknown exception");
    }
    m_sroSession.reset();
    m_navMeshSession.reset();
    m_sroInitialized = false;
    return false;
}

void EditorViewport::SetSroRegion(int rx, int ry) {
    if (!m_sroSession) return;
    m_sroRx = rx;
    m_sroRy = ry;
    m_sroSession->LoadRegion(rx, ry);
    m_sroSession->CollectPlacements(m_placements);
    g_activePlacements = &m_placements;
}

void EditorViewport::HandleViewportPick(EditorContext& ctx, bool focused, float vpW, float vpH) {
    if (!focused || !m_sroSession || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;
    if (ImGui::GetIO().WantCaptureMouse || vpW < 1 || vpH < 1) return;

    auto* rm = m_sroSession->Renderer().GetRenderManager();
    if (!rm) return;
    rm->ShowEventDecors = ctx.viewport.showEventDecors;
    rm->ShowParticles = ctx.viewport.showParticles;

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
        ctx.collisionEditor.selectedPlacementIdx = hitIdx;
        ctx.panels.collisionEditor = true;
        ctx.navmeshEditor.showObjectNavSurface = true;
        ctx.navmeshEditor.showObjectNavEdges = true;
        if (m_sroSession) {
            m_sroSession->EnsurePlacementCollision(m_placements[hitIdx]);
        }
        return;
    }

    float npcHitDist = 0.f;
    if (RaycastNpcs(ctx, rayPos, rayDir, 0.f, 0.f, npcHitDist)) {
        return;
    }

    if (ctx.activeTool == EditorToolType::NpcPlacement) {
        Vector3 ground = rayPos + rayDir * 500.f;
        ground.y = ctx.mouseWorldPos.y;
        HandleNpcPlacementClick(ctx, ground);
        return;
    }

    if (ctx.activeTool == EditorToolType::CollisionPaint && m_sroSession) {
        rm->BrushSize = static_cast<int>(ctx.navmeshEditor.brushSize);
        rm->PaintMode = ctx.navmeshEditor.paintMode;
        Vector3 hitPos;
        auto* regionMgr = m_sroSession->LegacyRegionManager();
        if (regionMgr && regionMgr->RaycastTerrain(rayPos, rayDir, 2000.0f, 5.0f, hitPos)) {
            bool setBlocked = (rm->PaintMode == 0);
            auto deltas = regionMgr->PaintNavmesh(hitPos.x, hitPos.z, setBlocked, rm->BrushSize, rm->PaintMode);
            if (!deltas.empty()) {
                regionMgr->PushAction(std::make_unique<PaintNavmeshFlagsAction>(m_sroRx, m_sroRy, deltas));
            }
        }
        return;
    }

    if (ctx.activeTool == EditorToolType::Select
        && ctx.panels.terrainNavPanel && ctx.sroRegionManager) {
        Vector3 hitPos;
        if (ctx.sroRegionManager->RaycastTerrain(rayPos, rayDir, 2000.0f, 5.0f, hitPos)) {
            if (auto* nav = ctx.sroRegionManager->GetNavMesh(m_sroRx, m_sroRy)) {
                int cellIdx = rm->RaycastNvmCells(hitPos, *nav, m_sroRx, m_sroRy, m_sroRx, m_sroRy);
                if (cellIdx >= 0) {
                    ctx.navmeshEditor.selectedCellIdx = cellIdx;
                    ctx.navmeshEditor.activeTab = NavMeshEditorTab::Analyze;
                    ctx.panels.terrainNavPanel = true;
                    return;
                }
                sro::LocalPos lp = sro::SceneSpace::SceneToLocal(hitPos.x, hitPos.z, m_sroRx, m_sroRy);
                int tx = static_cast<int>(lp.localX / 20.0f);
                int tz = static_cast<int>(lp.localZ / 20.0f);
                if (tx >= 0 && tx < 96 && tz >= 0 && tz < 96) {
                    ctx.navmeshEditor.selectedTileIdx = tz * 96 + tx;
                    ctx.navmeshEditor.activeTab = NavMeshEditorTab::TerrainData;
                    ctx.panels.terrainNavPanel = true;
                    return;
                }
            }
        }
    }
}

void EditorViewport::SyncClientContext(EditorContext& ctx) {
    if (ctx.sroClientLoaded && m_sroSession) {
        const int prevRx = m_sroRx;
        const int prevRy = m_sroRy;
        m_sroSession->UpdateCameraRegion(m_sroRx, m_sroRy, m_camera.Position);
        if (prevRx != m_sroRx || prevRy != m_sroRy) {
            m_sroSession->CollectPlacements(m_placements);
            g_activePlacements = &m_placements;
        }
        ctx.sroRegionRx = m_sroRx;
        ctx.sroRegionRy = m_sroRy;
        ctx.sroPlacements = &m_placements;
        ctx.sroRegionManager = m_sroSession->LegacyRegionManager();
        if (auto* rm = m_sroSession->Renderer().GetRenderManager()) {
            ctx.sroRenderManager = rm;
            ctx.sroMeshRenderer = rm->GetMeshRenderer();
            ctx.sroEffectRenderer = rm->GetEffectRenderer();
        }
        ctx.sroAssets = &m_sroSession->Assets();
        ctx.sroTextData = &m_sroSession->Assets().TextData();
        ctx.navMeshSession = m_navMeshSession.get();
        ctx.ensurePlacementCollision = [this](PlacementVM& vm) {
            if (m_sroSession) m_sroSession->EnsurePlacementCollision(vm);
        };
        if (auto* rm = ctx.sroRenderManager) {
            rm->BrushSize = static_cast<int>(ctx.navmeshEditor.brushSize);
            rm->PaintMode = ctx.navmeshEditor.paintMode;
        }
    } else {
        ctx.sroPlacements = nullptr;
        ctx.sroRegionManager = nullptr;
        ctx.sroRenderManager = nullptr;
        ctx.sroMeshRenderer = nullptr;
        ctx.sroEffectRenderer = nullptr;
        ctx.sroAssets = nullptr;
        ctx.sroTextData = nullptr;
        ctx.navMeshSession = nullptr;
        ctx.ensurePlacementCollision = nullptr;
    }
}

void EditorViewport::BeginClientLoad(const ClientLoadRequest& request) {
    m_clientLoadRequest = request;
    m_clientLoadStep = 0;
    m_clientLoadActive = true;
    m_clientLoadProgress = {};
    m_clientLoadProgress.active = true;
    m_clientLoadProgress.stage = "Preparing client load...";
    m_sroInitialized = false;

    if (!m_device || !FileExists(request.clientPath)) {
        m_clientLoadProgress.failed = true;
        m_clientLoadProgress.error = "Invalid client path";
        m_clientLoadProgress.active = false;
        m_clientLoadActive = false;
        return;
    }

    m_sroSession = std::make_unique<sro::SroWorldSession>();
    m_navMeshSession.reset();
    m_sroSession->PrepareClientShell(m_device, request.clientPath);
    m_clientPath = request.clientPath;
    m_sroRx = request.rx;
    m_sroRy = request.ry;
    m_clientLoadTotalSteps = m_sroSession->Assets().TotalLoadSteps() + 2;
}

bool EditorViewport::TickClientLoad(ClientLoadProgress& progress) {
    if (!m_clientLoadActive || !m_sroSession) return false;

    m_clientLoadProgress.active = true;
    m_clientLoadProgress.failed = false;

    if (m_sroSession->StepAssetLoad(m_clientLoadProgress)) {
        m_clientLoadProgress.fraction =
            static_cast<float>(m_sroSession->Assets().CompletedLoadSteps())
            / static_cast<float>((std::max)(1, m_sroSession->Assets().TotalLoadSteps()));
        progress = m_clientLoadProgress;
        return true;
    }

    if (m_clientLoadProgress.failed) {
        m_clientLoadActive = false;
        progress = m_clientLoadProgress;
        m_sroSession.reset();
        m_navMeshSession.reset();
        return false;
    }

    // Incremental region loading: one neighbor per tick so the progress bar
    // reports each navmesh/placement file individually instead of blocking.
    if (m_sroSession->IsFinalizeIdle()) {
        m_clientLoadProgress.step = 0;
        m_clientLoadProgress.totalSteps = 0;
        if (!m_sroSession->BeginFinalizeClientOpen(m_clientLoadRequest.rx, m_clientLoadRequest.ry, m_clientLoadProgress)) {
            m_clientLoadProgress.failed = true;
            m_clientLoadProgress.error = "Failed to begin region load";
            m_clientLoadActive = false;
            progress = m_clientLoadProgress;
            m_sroSession.reset();
            m_navMeshSession.reset();
            m_sroInitialized = false;
            return false;
        }
        progress = m_clientLoadProgress;
        return true;
    }

    if (!m_sroSession->IsFinalizeDone()) {
        if (m_sroSession->StepFinalizeClientOpen(m_clientLoadProgress)) {
            progress = m_clientLoadProgress;
            return true;
        }
    }

    if (m_clientLoadProgress.failed) {
        m_clientLoadActive = false;
        progress = m_clientLoadProgress;
        m_sroSession.reset();
        m_navMeshSession.reset();
        m_sroInitialized = false;
        return false;
    }

    m_sroSession->CollectPlacements(m_placements);
    g_activePlacements = &m_placements;
    m_camera.SetLoadRadius(m_sroSession->LoadRadius());
    if (m_clientLoadRequest.useRestoreCamera) {
        m_camera.Position = m_clientLoadRequest.restorePosition;
        m_camera.Yaw = m_clientLoadRequest.restoreYaw;
        m_camera.Pitch = m_clientLoadRequest.restorePitch;
    } else {
        m_camera.Position = Vector3(960.0f, 150.0f, 960.0f);
    }
    m_sroInitialized = true;
    if (!m_navMeshSession)
        m_navMeshSession = std::make_unique<sro::nav::NavMeshSession>();
    m_navMeshSession->SetClientPath(m_clientPath);
    m_navMeshSession->Scan();
    m_clientLoadProgress.done = true;
    m_clientLoadProgress.active = false;
    m_clientLoadProgress.fraction = 1.f;
    m_clientLoadProgress.stage = "Ready";
    m_clientLoadActive = false;
    progress = m_clientLoadProgress;
    return false;
}

void EditorViewport::Update(EditorContext& ctx, float dt, bool focused, bool rmbDown, float mouseDx, float mouseDy, const bool keys[256]) {
    SyncClientContext(ctx);

    if (ctx.navmeshEditor.navTopDownCamera) {
        m_camera.Position = Vector3(960.0f, 900.0f, 960.0f);
        m_camera.Pitch = -89.0f;
        m_camera.Yaw = -90.0f;
        m_camera.UpdateCameraVectors();
        ctx.navmeshEditor.navTopDownCamera = false;
    }

    if (!focused) {
        m_mouseCaptured = false;
        if (ctx.sroClientLoaded && m_sroSession) {
            ctx.mouseWorldPos = m_camera.Position;
        } else {
            float regionOffset = (ctx.activeRegionId - 25000) * 1920.0f;
            ctx.mouseWorldPos = m_camera.Position + Vector3(regionOffset, 0, 0);
        }
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
        ctx.mouseWorldPos = m_camera.Position;
        HandleViewportPick(ctx, focused, (float)m_framebuffer.Width(), (float)m_framebuffer.Height());
    } else {
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
        renderer.ShowInternalEdges = settings.showCollision;
        renderer.ShowGlobalEdges = settings.showCollision;
        renderer.ShowCells = false;
        renderer.WireframeMode = false;
        break;
    case ViewportMode::Wireframe:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowInternalEdges = false;
        renderer.ShowGlobalEdges = false;
        renderer.ShowCells = false;
        renderer.WireframeMode = true;
        break;
    case ViewportMode::Collision:
        renderer.ShowTerrain = false;
        renderer.ShowObjects = false;
        renderer.ShowWalkable = true;
        renderer.ShowBlocked = true;
        renderer.ShowInternalEdges = true;
        renderer.ShowGlobalEdges = true;
        renderer.ShowCells = true;
        renderer.WireframeMode = false;
        break;
    case ViewportMode::ZoneOverlay:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowInternalEdges = false;
        renderer.ShowGlobalEdges = false;
        renderer.ShowCells = false;
        renderer.WireframeMode = false;
        flags.drawZones = true;
        break;
    case ViewportMode::SpawnOverlay:
        renderer.ShowTerrain = true;
        renderer.ShowObjects = true;
        renderer.ShowWalkable = false;
        renderer.ShowBlocked = false;
        renderer.ShowInternalEdges = false;
        renderer.ShowGlobalEdges = false;
        renderer.ShowCells = false;
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
        renderer.ShowInternalEdges = false;
        renderer.ShowGlobalEdges = false;
        renderer.ShowCells = false;
        renderer.WireframeMode = false;
        flags.drawRegionDebug = true;
        break;
    case ViewportMode::NavEdit:
        renderer.ShowTerrain = false;
        renderer.ShowObjects = false;
        renderer.ShowWalkable = true;
        renderer.ShowBlocked = true;
        renderer.ShowInternalEdges = false;
        renderer.ShowGlobalEdges = false;
        renderer.ShowCells = false;
        renderer.WireframeMode = false;
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
        if (flags.drawNpcs && !(ctx.sroMeshRenderer && ctx.sroTextData)) {
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
    if (ctx.viewport.showNpcs && ctx.sroMeshRenderer)
        DrawNpcMeshes(ctx, view, proj, regionOffset, 0.0f);
    DrawEditorOverlays(ctx, m_device, view, proj, overlayFlags, regionOffset, 0.0f, 0, 0, 1);

    if (ctx.collisionEditor.showCollisionBoxes && ctx.sroPlacements) {
        if (auto* nvm = m_sroSession ? m_sroSession->Renderer().GetRenderManager()->GetNvmRenderer() : nullptr) {
            int16_t selUid = -1;
            if (ctx.selection && ctx.selection->kind == EntityKind::MapPlacement) {
                try { selUid = static_cast<int16_t>(std::stoi(ctx.selection->id)); } catch (...) {}
            }
            BmsDrawLayers bmsLayers;
            bmsLayers.wikiCollision = true;
            nvm->DrawObjectNavmeshes(view, proj, 0, 0, *ctx.sroPlacements, bmsLayers, selUid);
        }
    }

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

    const auto& L = ctx.navLayers;
    const bool anyNavLayer = L.showTerrainBlocked || L.showTerrainWalkable || L.showTileMapOverlay
        || L.showHeightMapSurface || L.showHeightMapWireframe || L.showPlaneMap
        || L.showCellQuads || L.showInternalEdges || L.showGlobalEdges
        || L.showBmsWikiCollision || L.showBmsPassabilityClass || L.showBmsBuilding
        || L.showBmsPlatform || L.showBmsEdgeDebug || L.showLinkEdges;
    const bool showTerrainNav = ctx.panels.terrainNavPanel || ctx.panels.navLayersPanel || anyNavLayer;
    const bool navEditMode = ctx.viewport.viewMode == ViewportMode::NavEdit;
    if (showTerrainNav || navEditMode) {
        renderer.ShowWalkable = L.showTerrainWalkable || navEditMode;
        renderer.ShowBlocked = L.showTerrainBlocked || navEditMode;
        renderer.ShowInternalEdges = L.showInternalEdges || navEditMode;
        renderer.ShowGlobalEdges = L.showGlobalEdges;
        renderer.ShowCells = L.showCellQuads || navEditMode;
        if ((L.hideWorldGeometry || ctx.navmeshEditor.navHideWorldGeometry) && navEditMode) {
            renderer.ShowTerrain = false;
            renderer.ShowObjects = false;
        }
    }
    renderer.ShowEventDecors = ctx.viewport.showEventDecors;
    renderer.ShowParticles = ctx.viewport.showParticles;
    renderer.NewFrame();
    renderer.DrawSky(w, h);

    m_sroSession->BuildScenePlacements(m_placements,
        [&](int16_t uid) {
            return ctx.selection && ctx.selection->kind == EntityKind::MapPlacement
                && ctx.selection->id == std::to_string(uid);
        },
        m_scenePlacements);

    renderer.DrawScene(view, proj, m_camera.Position, m_sroRx, m_sroRy,
        m_sroSession->LoadRadius(), frustum, m_scenePlacements);

    if (ctx.viewport.showGrid || navEditMode) DrawGrid(m_device, view, proj, 0, 0);
    if (ctx.viewport.showRegionBounds || ctx.viewport.viewMode == ViewportMode::RegionDebug || navEditMode) {
        DrawRegionBounds(m_device, view, proj, m_sroRx, m_sroRy, m_sroRx, m_sroRy);
    }

    DrawEditorOverlays(ctx, m_device, view, proj, overlayFlags, 0.0f, 0.0f,
        m_sroRx, m_sroRy, m_sroSession->LoadRadius());

    if (ctx.viewport.showNpcs && ctx.sroMeshRenderer)
        DrawNpcMeshes(ctx, view, proj, 0.0f, 0.0f);

    // Real per-object RTNavMeshObj (BMS NavMeshOffset CellTri + edges) and the
    // tile blockers + LinkEdges -- all read straight from disk, nothing invented.
    if (m_sroSession) {
        if (auto* rm = m_sroSession->Renderer().GetRenderManager()) {
            if (auto* nvm = rm->GetNvmRenderer()) {
                auto* regionMgr = m_sroSession->LegacyRegionManager();

                if (regionMgr) {
                    if (auto* nav = regionMgr->GetNavMesh(m_sroRx, m_sroRy)) {
                        if (L.showTileMapOverlay)
                            nvm->DrawTileMapOverlay(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy,
                                ctx.navmeshEditor.tileColorMode, ctx.navmeshEditor.selectedTileIdx);
                        else if (ctx.navmeshEditor.selectedTileIdx >= 0)
                            nvm->DrawSelectedTile(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy,
                                                  ctx.navmeshEditor.selectedTileIdx);
                        if (L.showHeightMapSurface || L.showHeightMapWireframe)
                            nvm->DrawHeightMapSurface(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy,
                                                      L.showHeightMapSurface, L.showHeightMapWireframe);
                        if (L.showPlaneMap)
                            nvm->DrawPlaneMap(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy);
                        if (!ctx.navmeshEditor.pathfindResult.empty())
                            nvm->DrawPathfindCells(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy,
                                                   ctx.navmeshEditor.pathfindResult);
                    }
                }

                const bool anyBms = L.showBmsWikiCollision || L.showBmsPassabilityClass
                    || L.showBmsBuilding || L.showBmsPlatform || L.showBmsEdgeDebug
                    || ctx.collisionEditor.showCollisionBoxes;
                if (anyBms && ctx.sroPlacements) {
                    int16_t selUid = -1;
                    if (ctx.selection && ctx.selection->kind == EntityKind::MapPlacement) {
                        try { selUid = static_cast<int16_t>(std::stoi(ctx.selection->id)); } catch (...) {}
                    }
                    m_sroSession->EagerLoadRegionPlacementCollision(m_sroRx, m_sroRy, m_placements);
                    if (selUid >= 0) {
                        for (auto& p : m_placements) {
                            if (p.Object.UID == selUid)
                                m_sroSession->EnsurePlacementCollision(p);
                        }
                    }
                    BmsDrawLayers bmsLayers;
                    bmsLayers.wikiCollision = L.showBmsWikiCollision || ctx.collisionEditor.showCollisionBoxes;
                    bmsLayers.passabilityClass = L.showBmsPassabilityClass;
                    bmsLayers.showBuilding = L.showBmsBuilding;
                    bmsLayers.showPlatform = L.showBmsPlatform;
                    bmsLayers.edgeDebug = L.showBmsEdgeDebug;
                    nvm->DrawObjectNavmeshes(view, proj, m_sroRx, m_sroRy, *ctx.sroPlacements, bmsLayers, selUid);
                }

                if (L.showLinkEdges && regionMgr && ctx.sroPlacements) {
                    if (auto* nav = regionMgr->GetNavMesh(m_sroRx, m_sroRy))
                        nvm->DrawLinkEdges(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy, *ctx.sroPlacements);
                }

                if (L.showCellQuads && regionMgr) {
                    if (auto* nav = regionMgr->GetNavMesh(m_sroRx, m_sroRy))
                        nvm->DrawCellsAndSelection(view, proj, m_sroRx, m_sroRy, *nav, m_sroRx, m_sroRy,
                            ctx.navmeshEditor.selectedCellIdx,
                            ctx.navmeshEditor.selectedGlobalEdgeIdx >= 0 ? ctx.navmeshEditor.selectedGlobalEdgeIdx
                                                                         : ctx.navmeshEditor.selectedInternalEdgeIdx,
                            ctx.navmeshEditor.selectedGlobalEdgeIdx >= 0);
                }

                if ((showTerrainNav || navEditMode) && regionMgr) {
                    (void)regionMgr;
                }

                if (L.showNeighborRegions)
                    nvm->DrawNeighborRegionFrames(view, proj, m_sroRx, m_sroRy, m_sroSession->LoadRadius());
            }

            if (ctx.loadedDof && (ctx.panels.terrainNavPanel || ctx.panels.dungeonNavPanel || ctx.panels.navLayersPanel)) {
                if (auto* dof = rm->GetDofRenderer()) {
                    dof->Draw(view, proj, *ctx.loadedDof,
                              ctx.navLayers.showDofBlocks, ctx.navLayers.showDofVoxels,
                              ctx.navLayers.showDofLinks, ctx.navLayers.showDofObjects,
                              ctx.selectedDofBlockIdx);
                }
            }

            if (ctx.panels.aiNavDataPanel && ctx.navMeshSession) {
                const uint16_t rid = ctx.aiNavDataEditor.selectedRegionId;
                if (auto* aiNav = ctx.navMeshSession->GetAiNavData(rid)) {
                    static AiNavDataRenderer aiRenderer;
                    aiRenderer.Initialize(m_device);
                    aiRenderer.Draw(view, proj, *aiNav, true, true);
                }
            }
        }
    }
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

static std::string ResolveNpcModelPath(const EditorContext& ctx, const NPC& npc) {
    if (!ctx.sroTextData) return {};
    if (const auto* ref = ctx.sroTextData->FindByCodeName(npc.codeName))
        return ctx.sroTextData->ResolvePrimaryModelPath(*ref);
    return {};
}

void EditorViewport::DrawNpcMeshes(EditorContext& ctx, const Matrix4& view, const Matrix4& proj,
                                   float /*regionOffsetX*/, float /*regionOffsetZ*/) {
    if (!ctx.sroMeshRenderer || !ctx.viewport.showNpcs) return;
    auto* mr = ctx.sroMeshRenderer;

    int centerRx = 0;
    int centerRy = 0;
    int loadRadius = 1;
    if (ctx.sroClientLoaded && m_sroSession) {
        centerRx = m_sroRx;
        centerRy = m_sroRy;
        loadRadius = m_sroSession->LoadRadius();
    } else {
        EditorContext::DecodeRegionId(ctx.activeRegionId, centerRx, centerRy);
    }

    const bool clientPlacements = ctx.sroClientLoaded && ctx.sroAssets
        && ctx.sroAssets->Placements().Loaded() && ctx.sroTextData;

    mr->BeginBatch(view, proj, 0, false, true);

    if (clientPlacements) {
        for (int drx = -loadRadius; drx <= loadRadius; ++drx) {
            for (int dry = -loadRadius; dry <= loadRadius; ++dry) {
                const int rx = centerRx + drx;
                const int ry = centerRy + dry;
                const int regionId = EditorContext::EncodeRegionId(rx, ry);

                std::vector<sro::NpcPlacement> npcs;
                ctx.sroAssets->Placements().CollectNpcsForRegion(regionId, npcs);
                for (const auto& pl : npcs) {
                    const auto* ref = ctx.sroTextData->FindByServiceId(pl.serviceId);
                    if (!ref) continue;
                    const std::string modelPath = ctx.sroTextData->ResolvePrimaryModelPath(*ref);
                    if (modelPath.empty()) continue;

                    const Vector3 pos = sro::SceneSpace::ObjectWorldPosition(pl.localPos, rx, ry, centerRx, centerRy);
                    mr->DrawModel(modelPath, pos, 0.f, false, false, view, proj);
                }
            }
        }
    }

    for (const auto& region : ctx.world.regions) {
        int rx = 0;
        int ry = 0;
        EditorContext::DecodeRegionId(region.id, rx, ry);

        for (const auto& npc : region.npcs) {
            const std::string modelPath = ResolveNpcModelPath(ctx, npc);
            if (modelPath.empty()) continue;

            const Vector3 pos = sro::SceneSpace::ObjectWorldPosition(
                npc.transform.position, rx, ry, centerRx, centerRy);
            const bool selected = ctx.selection && ctx.selection->kind == EntityKind::Npc
                && ctx.selection->id == npc.id && ctx.selection->regionId == region.id;
            mr->DrawModel(modelPath, pos, npc.transform.rotation.y, selected, false, view, proj);
        }
    }

    mr->EndBatch();

    if (ctx.viewport.showTeleports && clientPlacements && ctx.sroTextData && m_device) {
        for (int drx = -loadRadius; drx <= loadRadius; ++drx) {
            for (int dry = -loadRadius; dry <= loadRadius; ++dry) {
                const int rx = centerRx + drx;
                const int ry = centerRy + dry;
                const int regionId = EditorContext::EncodeRegionId(rx, ry);

                std::vector<sro::TeleportPlacement> teleports;
                ctx.sroAssets->Placements().CollectTeleportsForRegion(regionId, teleports);
                for (const auto& tp : teleports) {
                    const Vector3 pos = sro::SceneSpace::ObjectWorldPosition(tp.localPos, rx, ry, centerRx, centerRy);
                    DrawMarkerCross(m_device, view, proj, pos.x, pos.y, pos.z, 18.f,
                        D3DCOLOR_RGBA(200, 120, 255, 255));

                    const auto* ref = ctx.sroTextData->FindByServiceId(tp.serviceId);
                    if (!ref) continue;
                    const std::string modelPath = ctx.sroTextData->ResolvePrimaryModelPath(*ref);
                    if (modelPath.empty()) continue;

                    mr->BeginBatch(view, proj, 0, false, true);
                    mr->DrawModel(modelPath, pos, 0.f, false, false, view, proj);
                    mr->EndBatch();
                }
            }
        }
    }
}

bool EditorViewport::RaycastNpcs(EditorContext& ctx, const Vector3& rayPos, const Vector3& rayDir,
                               float regionOffsetX, float regionOffsetZ, float& outDist) {
    float bestDist = 1e9f;
    std::string bestId;
    int bestRegion = 0;

    for (const auto& region : ctx.world.regions) {
        float rOffset = (region.id - 25000) * 1920.0f;
        float baseX = regionOffsetX + rOffset;
        float baseZ = regionOffsetZ;

        for (const auto& npc : region.npcs) {
            Vector3 center(baseX + npc.transform.position.x,
                npc.transform.position.y + 1.5f,
                baseZ + npc.transform.position.z);
            Vector3 oc = rayPos - center;
            float b = oc.x * rayDir.x + oc.y * rayDir.y + oc.z * rayDir.z;
            float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - 2.0f * 2.0f;
            float disc = b * b - c;
            if (disc < 0.f) continue;
            float t = -b - sqrtf(disc);
            if (t > 0.f && t < bestDist) {
                bestDist = t;
                bestId = npc.id;
                bestRegion = region.id;
            }
        }
    }

    if (!bestId.empty()) {
        outDist = bestDist;
        ctx.Select({EntityKind::Npc, bestRegion, bestId});
        return true;
    }
    return false;
}

void EditorViewport::HandleNpcPlacementClick(EditorContext& ctx, const Vector3& worldPos) {
    Region* region = ctx.world.FindRegion(ctx.activeRegionId);
    if (!region) return;

    NPC npc;
    npc.id = "npc_" + std::to_string(region->npcs.size() + 1) + "_" + std::to_string(ctx.activeRegionId);
    npc.codeName = ctx.npcEditorState.newCodeName.empty() ? "NPC_CH_SMITH" : ctx.npcEditorState.newCodeName;
    npc.regionId = ctx.activeRegionId;
    npc.transform.position = worldPos;
    region->npcs.push_back(npc);
    ctx.MarkModified();
    ctx.Select({EntityKind::Npc, ctx.activeRegionId, npc.id});
    Logger::Instance().Info("Placed NPC " + npc.codeName + " at region " + std::to_string(ctx.activeRegionId));
}
