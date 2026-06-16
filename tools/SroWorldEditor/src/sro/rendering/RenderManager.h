#pragma once
#include <d3d9.h>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cmath>
#include "core/SceneSpace.h"
#include "formats/NavMeshFormat.h"
#include "Core/Math.h"
#include "Rendering/Camera.h"
#include "Rendering/Texture.h"
#include "Rendering/TerrainRenderer.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/NvmRenderer.h"
#include "Rendering/WorldMapControl.h"
#include "Rendering/MinimapWidget.h"
#include "sro_map/MapTextureCache.h"
#include "Rendering/SkyRenderer.h"
#include "Core/Log.h"
#include "world/PlacementFilter.h"
#include "ViewportFramebuffer.h"

// Define a simplified Placement info struct matching the VM in main
struct RenderPlacementVM {
    Vector3 Position; // Rel Position
    float Yaw;
    std::string BsrPath;
    bool IsSelected;
    int RegionX;
    int RegionY;
    int Lod;
    int16_t ObjectUID;
    uint32_t ObjID;
};

class RenderManager {
public:
    struct MapRenderParams {
        bool drawMinimapHud = false;
        float hudX = 0.0f;
        float hudY = 0.0f;
        float hudSize = 0.0f;
    };

private:
    LPDIRECT3DDEVICE9 m_device = nullptr;
    std::unique_ptr<TextureManager> m_texManager;
    std::unique_ptr<MapTextureCache> m_mapTextureCache;
    std::unique_ptr<TerrainRenderer> m_terrainRenderer;
    std::unique_ptr<MeshRenderer> m_meshRenderer;
    std::unique_ptr<NvmRenderer> m_nvmRenderer;
    std::unique_ptr<WorldMapControl> m_worldMapControl;
    std::unique_ptr<MinimapWidget> m_minimapWidget;
    ViewportFramebuffer m_mapFramebuffer;
    UINT m_mapFramebufferLastW = 0;
    UINT m_mapFramebufferLastH = 0;
    float m_mapRttLastPanX = 0.0f;
    float m_mapRttLastPanY = 0.0f;
    float m_mapRttLastZoom = 0.0f;
    int m_mapRttLastMinMx = 0;
    int m_mapRttLastMaxMy = 0;
    int m_mapRttLastMapStyle = 0;

public:
    // Constants for SRO coordinate math
    static constexpr float REGION_SIZE = 1920.0f;
    static constexpr float SRO_REGION_SIZE = 192.0f;
    static constexpr int CENTER_RX = 92;
    static constexpr int CENTER_RY = 135;

    // Visibility states
    bool ShowTerrain = true;
    bool ShowObjects = true;
    bool ShowWalkable = true;
    bool ShowBlocked = true;
    bool ShowEdges = true;
    bool ShowEventDecors = false;
    int TimeOfDay = 0; // 0 = Warm Sunny Day, 1 = Midnight Night
    std::set<int16_t> HiddenObjectUIDs;
    bool ShowHiddenAsWireframe = true;
    bool WireframeMode = false;

    enum class EditorMode {
        ObjectMode,
        TerrainElevMode,
        TerrainTexMode,
        WaterMode,
        NavmeshPaintMode,
        EdgeMode
    };
    EditorMode ActiveMode = EditorMode::ObjectMode;

    enum class ElevBrushType {
        Raise,
        Lower,
        Smooth,
        Flatten
    };
    ElevBrushType ActiveElevBrush = ElevBrushType::Raise;
    float ElevBrushStrength = 1.5f;
    float ElevBrushHeightVal = 0.0f; // Target height for Flatten

    uint16_t ActivePaintTextureId = 0;
    int ActivePaintTextureScale = 16;

    // Snapping Settings
    bool SnapToGrid = false;
    float GridSize = 1.0f;
    bool SnapToTerrain = true;
    bool SnapToRotation = false;
    float RotationSnapAngle = 45.0f; // in degrees

    // Brush Settings
    bool PaintEnabled = false;
    int PaintMode = 0; // 0 = Blocked, 1 = Walkable
    float BrushSize = 1.0f;

    // Navmesh Selection State
    int SelectedNvmEdgeIdx = -1;
    bool SelectedNvmEdgeIsGlobal = false;
    int SelectedNvmCellIdx = -1;

    RenderManager() = default;
    ~RenderManager() = default;

    static sro::LocalPos SceneToLocal(float sceneX, float sceneZ, int centerRx, int centerRy) {
        return sro::SceneSpace::SceneToLocal(sceneX, sceneZ, centerRx, centerRy);
    }

    static sro::WorldPos LocalToWorld(const sro::LocalPos& lp) {
        return sro::SceneSpace::LocalToWorld(lp);
    }

    static sro::WorldPos SceneToWorld(float sceneX, float sceneZ, int centerRx, int centerRy) {
        return sro::SceneSpace::SceneToWorld(sceneX, sceneZ, centerRx, centerRy);
    }

    static float GetSceneOffsetX(int ry, int centerRy) {
        return sro::SceneSpace::SceneOffsetX(ry, centerRy);
    }

    static float GetSceneOffsetZ(int rx, int centerRx) {
        return sro::SceneSpace::SceneOffsetZ(rx, centerRx);
    }

    static Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4& viewProj, float width, float height, float vpX = 0.0f, float vpY = 0.0f) {
        Vector4 clipPos;
        clipPos.x = worldPos.x * viewProj.m[0][0] + worldPos.y * viewProj.m[1][0] + worldPos.z * viewProj.m[2][0] + viewProj.m[3][0];
        clipPos.y = worldPos.x * viewProj.m[0][1] + worldPos.y * viewProj.m[1][1] + worldPos.z * viewProj.m[2][1] + viewProj.m[3][1];
        clipPos.z = worldPos.x * viewProj.m[0][2] + worldPos.y * viewProj.m[1][2] + worldPos.z * viewProj.m[2][2] + viewProj.m[3][2];
        clipPos.w = worldPos.x * viewProj.m[0][3] + worldPos.y * viewProj.m[1][3] + worldPos.z * viewProj.m[2][3] + viewProj.m[3][3];

        if (clipPos.w <= 0.0f) return Vector2(-1.0f, -1.0f);

        float ndcX = clipPos.x / clipPos.w;
        float ndcY = clipPos.y / clipPos.w;

        float screenX = vpX + (ndcX + 1.0f) * 0.5f * width;
        float screenY = vpY + (1.0f - ndcY) * 0.5f * height;

        return Vector2(screenX, screenY);
    }

    void Initialize(LPDIRECT3DDEVICE9 device, const std::wstring& clientPath) {
        m_device = device;
        m_texManager = std::make_unique<TextureManager>(device);
        m_mapTextureCache = std::make_unique<MapTextureCache>(device);
        m_terrainRenderer = std::make_unique<TerrainRenderer>(device, clientPath, m_texManager.get());
        m_meshRenderer = std::make_unique<MeshRenderer>(device, clientPath, m_texManager.get());
        m_nvmRenderer = std::make_unique<NvmRenderer>(device);
        m_worldMapControl = std::make_unique<WorldMapControl>(device, m_texManager.get(), m_mapTextureCache.get());
        m_minimapWidget = std::make_unique<MinimapWidget>(device, m_mapTextureCache.get());
        m_minimapWidget->SetOverlayTextureManager(m_texManager.get(), clientPath);
        m_minimapWidget->SetPathLookup([wmc = m_worldMapControl.get()](int rx, int ry) {
            return wmc ? wmc->PathForMinimapRegion(rx, ry) : std::nullopt;
        });
        m_worldMapControl->SetMinimapIndexListener([this]() {
            if (m_minimapWidget) m_minimapWidget->NotifyIndexUpdated();
            if (m_worldMapControl) m_worldMapControl->MarkMapRenderDirty();
        });
        m_mapFramebuffer.Initialize(device);
    }

    void Cleanup() {
        m_terrainRenderer.reset();
        m_meshRenderer.reset();
        m_nvmRenderer.reset();
        m_worldMapControl.reset();
        m_minimapWidget.reset();
        m_mapFramebuffer.Shutdown();
        m_mapTextureCache.reset();
        m_texManager.reset();
    }

    void OnDeviceLost() {
        m_mapFramebuffer.OnDeviceLost();
        m_mapFramebufferLastW = 0;
        m_mapFramebufferLastH = 0;
        m_mapRttLastPanX = 0.0f;
        m_mapRttLastPanY = 0.0f;
        m_mapRttLastZoom = 0.0f;
        m_mapRttLastMinMx = 0;
        m_mapRttLastMaxMy = 0;
        m_mapRttLastMapStyle = 0;
        if (m_worldMapControl) m_worldMapControl->MarkMapRenderDirty();
    }

    TextureManager* GetTextureManager() const { return m_texManager.get(); }
    MapTextureCache* GetMapTextureCache() const { return m_mapTextureCache.get(); }
    TerrainRenderer* GetTerrainRenderer() const { return m_terrainRenderer.get(); }
    MeshRenderer* GetMeshRenderer() const { return m_meshRenderer.get(); }
    NvmRenderer* GetNvmRenderer() const { return m_nvmRenderer.get(); }
    WorldMapControl* GetWorldMapControl() const { return m_worldMapControl.get(); }
    MinimapWidget* GetMinimapWidget() const { return m_minimapWidget.get(); }

    void NewFrame() {
        if (m_mapTextureCache) {
            m_mapTextureCache->BeginFrame();
        }
        if (m_worldMapControl) {
            m_worldMapControl->TickBackground();
        }
        if (m_texManager) {
            m_texManager->NewFrame();
            if (m_texManager->GetCacheSize() > 400) {
                m_texManager->Clear();
                if (m_terrainRenderer) {
                    m_terrainRenderer->ClearLoadedTextures();
                }
                if (m_worldMapControl) {
                    m_worldMapControl->InvalidateOverlayTextures();
                }
                if (m_minimapWidget) {
                    m_minimapWidget->InvalidateCachedTextures();
                }
                LogMsg("[RenderManager] Texture cache size exceeded 400. Automatically cleared texture cache to prevent VRAM overflow.");
            }
        }
    }

    void DrawSky(float width, float height) {
        DWORD colTop = (TimeOfDay == 0) ? D3DCOLOR_XRGB(75, 145, 235) : D3DCOLOR_XRGB(10, 14, 25);
        DWORD colBottom = (TimeOfDay == 0) ? D3DCOLOR_XRGB(165, 205, 240) : D3DCOLOR_XRGB(35, 42, 58);
        DrawSkyGradient(m_device, width, height, colTop, colBottom);
    }

    void Draw3DScene(const Matrix4& view, const Matrix4& proj, const Vector3& cameraPos, int currentRx, int currentRy, int loadRadius, 
                     const CameraFrustum& frustum, const std::vector<RenderPlacementVM>& placements) {
        m_device->SetRenderState(D3DRS_FOGENABLE, TRUE);
        
        DWORD fogCol = (TimeOfDay == 0) ? D3DCOLOR_XRGB(165, 205, 240) : D3DCOLOR_XRGB(35, 42, 58);
        m_device->SetRenderState(D3DRS_FOGCOLOR, fogCol);
        
        // SRO uses Vertex Fog (robustly supported and aligns perfectly with fixed function vertices)
        m_device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        m_device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        
        float fogEnd = 2800.0f;
        if (loadRadius == 0) fogEnd = 1200.0f;
        else if (loadRadius == 1) fogEnd = 3200.0f;
        else if (loadRadius == 2) fogEnd = 5200.0f;
        else if (loadRadius == 3) fogEnd = 7200.0f;
        else if (loadRadius == 4) fogEnd = 9200.0f;
        float fogStart = fogEnd * 0.30f;
        
        m_device->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&fogStart);
        m_device->SetRenderState(D3DRS_FOGEND, *(DWORD*)&fogEnd);

        if (WireframeMode) {
            m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        }

        if (ShowTerrain && m_terrainRenderer) {
            m_terrainRenderer->Draw(view, proj, currentRx, currentRy, loadRadius, frustum, TimeOfDay, WireframeMode);
        }

        if (ShowObjects && m_meshRenderer) {
            m_meshRenderer->BeginBatch(view, proj, TimeOfDay, WireframeMode);
            for (const auto& vm : placements) {
                // Skip rendering if hidden, unless ShowHiddenAsWireframe is enabled or it is selected
                bool isHidden = (HiddenObjectUIDs.count(vm.ObjectUID) > 0);
                if (isHidden && !ShowHiddenAsWireframe && !vm.IsSelected) continue;
                if (!ShowEventDecors && sro::IsEventOrSeasonalDecor(vm.BsrPath) && !vm.IsSelected) continue;

                float oX = (vm.RegionY - currentRy) * REGION_SIZE;
                float oZ = (vm.RegionX - currentRx) * REGION_SIZE;

                if (!frustum.IsBoxVisible(Vector3(oX, -1000.0f, oZ), Vector3(oX + REGION_SIZE, 3000.0f, oZ + REGION_SIZE))) {
                    continue;
                }

                Vector3 worldPos(vm.Position.x + oX, vm.Position.y, vm.Position.z + oZ);
                
                // Distance to camera for LOD culling
                float dx = worldPos.x - cameraPos.x;
                float dy = worldPos.y - cameraPos.y;
                float dz = worldPos.z - cameraPos.z;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                
                // Skip drawing small/medium details at long distances to save performance
                if (vm.Lod == 0 && dist > 1400.0f) continue;
                if (vm.Lod == 1 && dist > 2600.0f) continue;
                if (vm.Lod == 2 && dist > 4500.0f) continue;

                m_meshRenderer->DrawModel(vm.BsrPath, worldPos, vm.Yaw, vm.IsSelected, isHidden, view, proj);
            }
            m_meshRenderer->EndBatch();
        }

        if (m_nvmRenderer) {
            m_nvmRenderer->Draw(view, proj, currentRx, currentRy, loadRadius, ShowWalkable, ShowBlocked, ShowEdges, frustum);
        }

        if (WireframeMode) {
            m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        }

        m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }

    int RaycastObjects(const Vector3& rayPos, const Vector3& rayDir, const std::vector<RenderPlacementVM>& placements, int currentRx, int currentRy, float& outDist) {
        int closestIdx = -1;
        float minDist = 999999.0f;

        for (int i = 0; i < (int)placements.size(); ++i) {
            const auto& vm = placements[i];
            
            // Skip hidden objects for raycast selection unless they are visible as wireframe
            if (HiddenObjectUIDs.count(vm.ObjectUID) > 0 && !ShowHiddenAsWireframe) continue;
            if (!ShowEventDecors && sro::IsEventOrSeasonalDecor(vm.BsrPath)) continue;

            float oX = (vm.RegionY - currentRy) * REGION_SIZE;
            float oZ = (vm.RegionX - currentRx) * REGION_SIZE;
            Vector3 worldPos(vm.Position.x + oX, vm.Position.y, vm.Position.z + oZ);

            // Get model bounds
            Vector3 bMin(-20.0f, 0.0f, -20.0f);
            Vector3 bMax(20.0f, 40.0f, 20.0f);
            
            if (m_meshRenderer) {
                auto* modelRes = m_meshRenderer->PreloadModel(vm.BsrPath);
                if (modelRes && modelRes->MinBounds.x < modelRes->MaxBounds.x) {
                    bMin = modelRes->MinBounds;
                    bMax = modelRes->MaxBounds;
                }
            }

            // Transform ray to local space of the object (inverse Y rotation)
            Vector3 localRayPos = rayPos - worldPos;
            float cosY = cosf(vm.Yaw);
            float sinY = sinf(vm.Yaw);

            Vector3 localRayPosRot;
            localRayPosRot.x = localRayPos.x * cosY - localRayPos.z * sinY;
            localRayPosRot.y = localRayPos.y;
            localRayPosRot.z = localRayPos.x * sinY + localRayPos.z * cosY;

            Vector3 localRayDirRot;
            localRayDirRot.x = rayDir.x * cosY - rayDir.z * sinY;
            localRayDirRot.y = rayDir.y;
            localRayDirRot.z = rayDir.x * sinY + rayDir.z * cosY;

            float t1 = (bMin.x - localRayPosRot.x) / (localRayDirRot.x != 0.0f ? localRayDirRot.x : 1e-6f);
            float t2 = (bMax.x - localRayPosRot.x) / (localRayDirRot.x != 0.0f ? localRayDirRot.x : 1e-6f);
            float t3 = (bMin.y - localRayPosRot.y) / (localRayDirRot.y != 0.0f ? localRayDirRot.y : 1e-6f);
            float t4 = (bMax.y - localRayPosRot.y) / (localRayDirRot.y != 0.0f ? localRayDirRot.y : 1e-6f);
            float t5 = (bMin.z - localRayPosRot.z) / (localRayDirRot.z != 0.0f ? localRayDirRot.z : 1e-6f);
            float t6 = (bMax.z - localRayPosRot.z) / (localRayDirRot.z != 0.0f ? localRayDirRot.z : 1e-6f);

            float tmin = (std::max)((std::max)((std::min)(t1, t2), (std::min)(t3, t4)), (std::min)(t5, t6));
            float tmax = (std::min)((std::min)((std::max)(t1, t2), (std::max)(t3, t4)), (std::max)(t5, t6));

            if (tmax >= 0.0f && tmin <= tmax) {
                float hitVal = (tmin < 0.0f) ? tmax : tmin;
                if (hitVal < minDist) {
                    minDist = hitVal;
                    closestIdx = i;
                }
            }
        }

        outDist = minDist;
        return closestIdx;
    }

    static float DistancePointToSegment(const Vector2& P, const Vector2& A, const Vector2& B) {
        float l2 = (B.x - A.x) * (B.x - A.x) + (B.y - A.y) * (B.y - A.y);
        if (l2 == 0.0f) return sqrtf((P.x - A.x) * (P.x - A.x) + (P.y - A.y) * (P.y - A.y));
        float t = ((P.x - A.x) * (B.x - A.x) + (P.y - A.y) * (B.y - A.y)) / l2;
        t = (std::max)(0.0f, (std::min)(1.0f, t));
        float projX = A.x + t * (B.x - A.x);
        float projY = A.y + t * (B.y - A.y);
        return sqrtf((P.x - projX) * (P.x - projX) + (P.y - projY) * (P.y - projY));
    }

    int RaycastNvmEdges(const Vector2& mousePos, const Matrix4& viewProj, float width, float height, float vpX, float vpY,
                         const sro::formats::NavMesh& nav, int rx, int ry, int centerRx, int centerRy, bool& outIsGlobal) {
        float minScreenDist = 12.0f; // 12 pixels threshold
        int closestEdgeIdx = -1;
        bool closestIsGlobal = false;

        float rOffsetX = GetSceneOffsetX(ry, centerRy);
        float rOffsetZ = GetSceneOffsetZ(rx, centerRx);

        // 1. Draw all cells as white outlines
        for (int i = 0; i < (int)nav.GlobalEdges.size(); ++i) {
            const auto& edge = nav.GlobalEdges[i];
            float h0 = m_nvmRenderer ? m_nvmRenderer->GetHeight(edge.MinX, edge.MinY, nav.HeightMap) : 0.0f;
            float h1 = m_nvmRenderer ? m_nvmRenderer->GetHeight(edge.MaxX, edge.MaxY, nav.HeightMap) : 0.0f;

            Vector3 start3D(edge.MinX + rOffsetX, h0 + 0.2f, edge.MinY + rOffsetZ);
            Vector3 end3D(edge.MaxX + rOffsetX, h1 + 0.2f, edge.MaxY + rOffsetZ);

            Vector2 start2D = WorldToScreen(start3D, viewProj, width, height, vpX, vpY);
            Vector2 end2D = WorldToScreen(end3D, viewProj, width, height, vpX, vpY);

            if (start2D.x >= 0.0f && end2D.x >= 0.0f) {
                float dist = DistancePointToSegment(mousePos, start2D, end2D);
                if (dist < minScreenDist) {
                    minScreenDist = dist;
                    closestEdgeIdx = i;
                    closestIsGlobal = true;
                }
            }
        }

        // 2. Internal Edges
        for (int i = 0; i < (int)nav.InternalEdges.size(); ++i) {
            const auto& edge = nav.InternalEdges[i];
            float h0 = m_nvmRenderer ? m_nvmRenderer->GetHeight(edge.MinX, edge.MinY, nav.HeightMap) : 0.0f;
            float h1 = m_nvmRenderer ? m_nvmRenderer->GetHeight(edge.MaxX, edge.MaxY, nav.HeightMap) : 0.0f;

            Vector3 start3D(edge.MinX + rOffsetX, h0 + 0.2f, edge.MinY + rOffsetZ);
            Vector3 end3D(edge.MaxX + rOffsetX, h1 + 0.2f, edge.MaxY + rOffsetZ);

            Vector2 start2D = WorldToScreen(start3D, viewProj, width, height, vpX, vpY);
            Vector2 end2D = WorldToScreen(end3D, viewProj, width, height, vpX, vpY);

            if (start2D.x >= 0.0f && end2D.x >= 0.0f) {
                float dist = DistancePointToSegment(mousePos, start2D, end2D);
                if (dist < minScreenDist) {
                    minScreenDist = dist;
                    closestEdgeIdx = i;
                    closestIsGlobal = false;
                }
            }
        }

        outIsGlobal = closestIsGlobal;
        return closestEdgeIdx;
    }

    int RaycastNvmCells(const Vector3& hitPos, const sro::formats::NavMesh& nav, int rx, int ry, int centerRx, int centerRy) {
        sro::LocalPos lp = SceneToLocal(hitPos.x, hitPos.z, centerRx, centerRy);
        if (lp.rx != rx || lp.ry != ry) return -1;

        float lx = lp.localX;
        float lz = lp.localZ;

        int closestCellIdx = -1;
        float smallestArea = 999999.0f;

        for (int i = 0; i < (int)nav.Cells.size(); ++i) {
            const auto& cell = nav.Cells[i];
            if (lx >= cell.MinX && lx <= cell.MaxX && lz >= cell.MinY && lz <= cell.MaxY) {
                float area = (cell.MaxX - cell.MinX) * (cell.MaxY - cell.MinY);
                if (area < smallestArea) {
                    smallestArea = area;
                    closestCellIdx = i;
                }
            }
        }
        return closestCellIdx;
    }

    void Draw2DMap(float viewportW, float viewportH) {
        if (m_worldMapControl) {
            m_worldMapControl->Render2D(viewportW, viewportH);
        }
    }

    LPDIRECT3DTEXTURE9 RenderMapToTexture(float w, float h, const MapRenderParams& params) {
        if (!m_device || w < 1.0f || h < 1.0f) return nullptr;
        const UINT tw = static_cast<UINT>(w);
        const UINT th = static_cast<UINT>(h);
        const bool sizeChanged = (tw != m_mapFramebufferLastW || th != m_mapFramebufferLastH);
        if (sizeChanged) {
            m_mapFramebufferLastW = tw;
            m_mapFramebufferLastH = th;
            if (m_worldMapControl) m_worldMapControl->MarkMapRenderDirty();
        }

        if (m_worldMapControl) {
            const bool viewChanged =
                m_worldMapControl->m_panOffset.x != m_mapRttLastPanX ||
                m_worldMapControl->m_panOffset.y != m_mapRttLastPanY ||
                m_worldMapControl->m_zoom != m_mapRttLastZoom ||
                m_worldMapControl->BoundsMinMx() != m_mapRttLastMinMx ||
                m_worldMapControl->BoundsMaxMy() != m_mapRttLastMaxMy ||
                m_worldMapControl->MapStyle != m_mapRttLastMapStyle;
            if (viewChanged) m_worldMapControl->MarkMapRenderDirty();
        }

        if (!sizeChanged && m_worldMapControl && !m_worldMapControl->IsMapRenderDirty() &&
            m_worldMapControl->PendingTextureLoads() == 0) {
            if (m_mapFramebuffer.EnsureSize(tw, th)) {
                return m_mapFramebuffer.ColorTexture();
            }
        }

        if (!m_mapFramebuffer.EnsureSize(tw, th)) return nullptr;
        if (!m_mapFramebuffer.BeginRender()) return nullptr;

        m_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                        D3DCOLOR_XRGB(16, 16, 18), 1.0f, 0);

        if (m_worldMapControl) m_worldMapControl->Render2D(w, h);

        m_mapFramebuffer.EndRender();
        if (m_worldMapControl) {
            m_mapRttLastPanX = m_worldMapControl->m_panOffset.x;
            m_mapRttLastPanY = m_worldMapControl->m_panOffset.y;
            m_mapRttLastZoom = m_worldMapControl->m_zoom;
            m_mapRttLastMinMx = m_worldMapControl->BoundsMinMx();
            m_mapRttLastMaxMy = m_worldMapControl->BoundsMaxMy();
            m_mapRttLastMapStyle = m_worldMapControl->MapStyle;
            m_worldMapControl->ClearMapRenderDirty();
        }
        return m_mapFramebuffer.ColorTexture();
    }
};
