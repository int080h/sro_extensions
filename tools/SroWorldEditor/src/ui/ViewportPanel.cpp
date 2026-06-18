#include "ui/UiCommon.h"
#include "ui/NavLayerUi.h"
#include "EditorPublic.h"
#include "ui/TileMapCanvas.h"
#include "sro/nav/ObjectNavSpatial.h"
#include "sro/rendering/RenderManager.h"
#include "imgui.h"
#include <cstring>
#include "imgui_internal.h"

void Ui::DrawPerformancePanel(const EditorContext& ctx) {
    ImGui::Begin("Performance", &const_cast<PanelVisibility&>(ctx.panels).performance);
    ImGui::Text("FPS: %.1f", ctx.fps);
    int objCount = 0, spawnCount = 0, npcCount = 0;
    for (const auto& r : ctx.world.regions) {
        objCount += (int)r.objects.size();
        spawnCount += (int)r.spawns.size();
        npcCount += (int)r.npcs.size();
    }
    ImGui::Text("Object count: %d", objCount);
    ImGui::Text("Spawn count: %d", spawnCount);
    ImGui::Text("NPC count: %d", npcCount);
    ImGui::Text("Region count: %d", (int)ctx.world.regions.size());
    if (ctx.sroRenderManager) {
        const auto& perf = ctx.sroRenderManager->GetLastFramePerf();
        ImGui::Text("Effect requests: %d", perf.EffectRequests);
        ImGui::Text("Active particles: %d", perf.ActiveParticles);
        ImGui::Text("Particle draw calls: %d", perf.ParticleDrawCalls);
    } else {
        ImGui::TextDisabled("Effect requests: N/A");
        ImGui::TextDisabled("Active particles: N/A");
        ImGui::TextDisabled("Particle draw calls: N/A");
    }
    ImGui::TextDisabled("Memory usage: N/A (M2+)");
    ImGui::End();
}

void Ui::SetupDefaultDockLayout(unsigned int dockspaceId) {
    ImGuiID id = dockspaceId;
    ImGui::DockBuilderRemoveNode(id);
    ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(id, ImGui::GetMainViewport()->Size);

    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 0.22f, nullptr, &id);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(id, ImGuiDir_Right, 0.25f, nullptr, &id);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(id, ImGuiDir_Down, 0.25f, nullptr, &id);
    ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.45f, nullptr, &dockLeft);

    ImGui::DockBuilderDockWindow("World Outliner", dockLeft);
    ImGui::DockBuilderDockWindow("Asset Browser", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Object Viewer", dockLeftBottom);
    ImGui::DockBuilderDockWindow("NPC Editor", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", id);
    ImGui::DockBuilderDockWindow("Properties", dockRight);
    ImGui::DockBuilderDockWindow("NavMesh Browser", dockLeft);
    ImGui::DockBuilderDockWindow("Terrain Nav", dockRight);
    ImGui::DockBuilderDockWindow("AI Nav Data", dockRight);
    ImGui::DockBuilderDockWindow("Dungeon Nav", dockRight);
    ImGui::DockBuilderDockWindow("Collision Editor", dockRight);
    ImGui::DockBuilderDockWindow("Validation", dockBottom);
    ImGui::DockBuilderDockWindow("Performance", dockBottom);
    ImGui::DockBuilderDockWindow("World Map", dockBottom);
    ImGui::DockBuilderFinish(id);
}

void Ui::DrawViewportPanel(EditorContext& ctx, Editor& editor) {
    if (!ImGui::Begin("Viewport", &ctx.panels.viewport)) { ImGui::End(); return; }

    const char* modes[] = {"Lit", "Wireframe", "BSR Collision", "Zone Overlay", "Spawn Overlay", "Region Debug", "Nav Edit"};
    int mode = static_cast<int>(ctx.viewport.viewMode);
    ImGui::SetNextItemWidth(140);
    if (ImGui::Combo("View Mode", &mode, modes, IM_ARRAYSIZE(modes)))
        ctx.viewport.viewMode = static_cast<ViewportMode>(mode);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Camera Speed", &ctx.viewport.cameraSpeed, 50.0f, 1000.0f);
    ImGui::SameLine();
    ImGui::Text("Region: %d", ctx.activeRegionId);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 1.0f || avail.y < 1.0f) {
        ImGui::End();
        return;
    }

    editor.RenderViewportScene(avail.x, avail.y);

    LPDIRECT3DTEXTURE9 tex = editor.Viewport().GetDisplayTexture();
    if (tex) {
        ImGui::Image((ImTextureID)(intptr_t)tex, avail, ImVec2(0, 0), ImVec2(1, 1));
    } else {
        ImGui::Dummy(avail);
    }

    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
    ImGui::InvisibleButton("viewport_canvas", avail, ImGuiButtonFlags_MouseButtonRight);
    bool focused = ImGui::IsItemFocused() || ImGui::IsItemHovered();
    editor.SetViewportFocused(focused);

    if (focused && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }

    if (!ctx.sroClientLoaded && tex) {
        const char* msg = "Sample world data — Import Client Data to load real SRO regions";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        ImVec2 pmin = ImGui::GetItemRectMin();
        ImVec2 pmax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(pmin.x + (pmax.x - pmin.x - ts.x) * 0.5f, pmax.y - ts.y - 8.0f),
            IM_COL32(200, 200, 200, 200), msg);
    }

    if ((ctx.panels.terrainNavPanel || ctx.panels.navLayersPanel) && ctx.sroClientLoaded) {
        ImVec2 pmin = ImGui::GetItemRectMin();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        char line0[128];
        snprintf(line0, sizeof(line0), "Region (%d,%d)  nv_%02x%02x.nvm",
            ctx.sroRegionRx, ctx.sroRegionRy, ctx.sroRegionRx, ctx.sroRegionRy);
        dl->AddText(ImVec2(pmin.x + 8, pmin.y + 8), IM_COL32(255, 255, 255, 230), line0);

        int terrainBlocked = 0;
        if (ctx.sroRegionManager) {
            if (auto* nav = ctx.sroRegionManager->GetNavMesh(ctx.sroRegionRx, ctx.sroRegionRy)) {
                for (const auto& t : nav->TileMap) {
                    if (Ui::IsTileBlocked(t.Flags)) ++terrainBlocked;
                }
            }
        }
        int bmsPlacements = 0;
        if (ctx.sroPlacements) {
            bmsPlacements = sro::nav::CountRegionPlacementsWithBms(
                *ctx.sroPlacements, ctx.sroRegionRx, ctx.sroRegionRy);
        }
        char stats[160];
        snprintf(stats, sizeof(stats), "Terrain blocked tiles: %d  |  Object placements with BMS: %d",
            terrainBlocked, bmsPlacements);
        dl->AddText(ImVec2(pmin.x + 8, pmin.y + 24), IM_COL32(200, 220, 255, 220), stats);

        const auto& L = ctx.navLayers;
        const bool anyNavLayer = L.showTerrainBlocked || L.showBmsWikiCollision || L.showBmsPassabilityClass
            || L.showInternalEdges || L.showGlobalEdges || L.showTileMapOverlay;
        float yLine = 44.0f;
        if (anyNavLayer || ctx.navmeshEditor.blockingViewActive) {
            dl->AddText(ImVec2(pmin.x + 8, pmin.y + yLine), IM_COL32(180, 255, 180, 230),
                "Nav layers active — see Nav Layers panel for legend");
            yLine += 18.0f;
        }
        if (ctx.navmeshEditor.selectedTileIdx >= 0) {
            char sel[256];
            int tx = ctx.navmeshEditor.selectedTileIdx % 96, tz = ctx.navmeshEditor.selectedTileIdx / 96;
            snprintf(sel, sizeof(sel), "Tile (%d,%d) idx=%d", tx, tz, ctx.navmeshEditor.selectedTileIdx);
            if (ctx.sroRegionManager && ctx.sroPlacements) {
                if (auto* nav = ctx.sroRegionManager->GetNavMesh(ctx.sroRegionRx, ctx.sroRegionRy)) {
                    if (ctx.navmeshEditor.selectedTileIdx < (int)nav->TileMap.size()) {
                        sro::nav::TileNavDiagnostics diag;
                        sro::nav::QueryTileNavDiagnostics(*nav, *ctx.sroPlacements,
                            ctx.sroRegionRx, ctx.sroRegionRy, tx, tz, diag);
                        char extra[160];
                        snprintf(extra, sizeof(extra), "  %s  CellID=%d",
                            sro::nav::PassabilityVerdict(diag.kind), diag.cellId);
                        strncat(sel, extra, sizeof(sel) - strlen(sel) - 1);
                    }
                }
            }
            dl->AddText(ImVec2(pmin.x + 8, pmin.y + yLine), IM_COL32(255, 255, 100, 230), sel);
            yLine += 18.0f;
        } else if (ctx.navmeshEditor.selectedCellIdx >= 0) {
            char sel[64];
            snprintf(sel, sizeof(sel), "Cell %d", ctx.navmeshEditor.selectedCellIdx);
            dl->AddText(ImVec2(pmin.x + 8, pmin.y + yLine), IM_COL32(255, 255, 100, 230), sel);
            yLine += 18.0f;
        }
    }

    ImGui::Text("Mouse world: %.0f, %.0f, %.0f", ctx.mouseWorldPos.x, ctx.mouseWorldPos.y, ctx.mouseWorldPos.z);
    ImGui::End();
}
