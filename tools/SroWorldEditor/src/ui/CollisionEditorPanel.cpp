#include "ui/UiCommon.h"
#include "EditorPublic.h"
#include "ui/TileMapCanvas.h"
#include "runtime/RegionManager.h"
#include "rendering/RenderManager.h"
#include "rendering/NvmRenderer.h"
#include "rendering/MeshRenderer.h"
#include "sro/nav/ObjectNavSpatial.h"
#include "sro/nav/NavMeshParseAudit.h"
#include "parsers/BsrParser.h"
#include "parsers/BmsParser.h"
#include "core/Logger.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cmath>

// Object Collision Inspector -- read-only view of the real game resource chain:
//   ObjID -> object.ifo -> .bsr (CollisionPath + collisionBox0/1) -> .bms NavMeshOffset (RTNavMeshObj)
//   or -> .cpd (collisionResourcePath) -> .bms NavMeshOffset
// Nothing here is invented or editor-editable; it is reported straight from disk.

static void DrawOverviewTab(EditorContext& ctx) {
    if (!ctx.sroPlacements) {
        ImGui::TextDisabled("No client data loaded.");
        return;
    }

    auto& placements = *ctx.sroPlacements;
    int total = (int)placements.size();
    int withNav = 0, withBox = 0, withPath = 0;
    long totalCells = 0, totalOutline = 0, totalInline = 0;

    for (const auto& p : placements) {
        if (p.Collision.NavMesh) {
            ++withNav;
            totalCells   += (long)p.Collision.NavMesh->Cells.size();
            totalOutline += (long)p.Collision.NavMesh->OutlineEdges.size();
            totalInline  += (long)p.Collision.NavMesh->InlineEdges.size();
        }
        if (p.Collision.HasAuthoredBox) ++withBox;
        if (!p.Collision.CollisionMeshPath.empty()) ++withPath;
    }

    if (ImGui::CollapsingHeader("Collision Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        ImGui::Text("Total placements: %d", total);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "With RTNavMeshObj (.bms NavMeshOffset): %d", withNav);
        ImGui::Text("With authored BSR collisionBox: %d", withBox);
        ImGui::Text("With resolved collision .bms path: %d", withPath);
        ImGui::Separator();
        ImGui::Text("Total CellTri cells: %ld", totalCells);
        ImGui::Text("Total outline edges: %ld", totalOutline);
        ImGui::Text("Total inline edges:  %ld", totalInline);
        ImGui::Unindent(8.0f);
    }

    if (ImGui::CollapsingHeader("Resource Chain", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        ImGui::TextWrapped("Each placement resolves collision via AssetID -> object.ifo -> .bsr "
                           "(CollisionPath + collisionBox0/1) -> .bms NavMeshOffset, or via .cpd "
                           "(collisionResourcePath) -> .bms. The .bms NavMeshOffset holds the "
                           "RTNavMeshCellTri cells + NavOutline/InlineEdges the game uses for AI "
                           "pathfinding on/around the object.");
        ImGui::Unindent(8.0f);
    }
}

static void DrawPlacementListTab(EditorContext& ctx) {
    if (!ctx.sroPlacements) {
        ImGui::TextDisabled("No client data loaded.");
        return;
    }

    auto& placements = *ctx.sroPlacements;
    auto& st = ctx.collisionEditor;

    ImGui::InputText("Search (UID/BsrPath)", st.searchFilter, sizeof(st.searchFilter));
    ImGui::Separator();

    ImGui::BeginChild("CollisionPlacList", ImVec2(320, -1), true);
    for (int i = 0; i < (int)placements.size(); ++i) {
        const auto& p = placements[i];
        if (st.searchFilter[0] != '\0') {
            std::string uidStr = std::to_string(p.Object.UID);
            if (!strstr(uidStr.c_str(), st.searchFilter) &&
                !strstr(p.BsrPath.c_str(), st.searchFilter)) continue;
        }

        const char* icon = p.Collision.NavMesh ? "[NAV]" :
                           (p.Collision.HasAuthoredBox ? "[BOX]" : "[--]");

        char label[256];
        snprintf(label, sizeof(label), "%s UID:%d %s", icon, p.Object.UID,
                 p.BsrPath.empty() ? "(no model)" : p.BsrPath.c_str());

        bool selected = (st.selectedPlacementIdx == i);
        if (ImGui::Selectable(label, selected)) {
            st.selectedPlacementIdx = i;
            ctx.Select({EntityKind::MapPlacement,
                EditorContext::EncodeRegionId(p.LoadedRx, p.LoadedRy),
                std::to_string(p.Object.UID)});
            ctx.navmeshEditor.showObjectNavSurface = true;
            ctx.navmeshEditor.showObjectNavEdges = true;
            if (ctx.ensurePlacementCollision)
                ctx.ensurePlacementCollision(const_cast<PlacementVM&>(p));
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("CollisionPlacProps", ImVec2(0, -1), true);
    if (st.selectedPlacementIdx >= 0 && st.selectedPlacementIdx < (int)placements.size()) {
        auto& p = placements[st.selectedPlacementIdx];
        if (ctx.ensurePlacementCollision)
            ctx.ensurePlacementCollision(p);

        ImGui::Text("UID: %d", p.Object.UID);
        ImGui::Text("AssetID (ObjID): %u", p.Object.ObjID);
        ImGui::Text("Model: %s", p.BsrPath.empty() ? "(none)" : p.BsrPath.c_str());
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", p.Object.PosX, p.Object.PosY, p.Object.PosZ);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Resource Chain", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(8.0f);
            ImGui::Text("BSR: %s", p.BsrPath.empty() ? "(unresolved)" : p.BsrPath.c_str());
            ImGui::Text("Collision .bms: %s", p.Collision.CollisionMeshPath.empty()
                                                ? "(none / no NavMeshOffset)" : p.Collision.CollisionMeshPath.c_str());
            ImGui::Text("Authored collisionBox0/1: %s", p.Collision.HasAuthoredBox ? "yes" : "no");
            if (p.Collision.RequireCollisionMatrix)
                ImGui::Text("RequireCollisionMatrix: yes (transform in BSR)");
            ImGui::Unindent(8.0f);
        }

        if (ImGui::CollapsingHeader("RTNavMeshObj (.bms NavMeshOffset)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(8.0f);
            const BmsNavMesh* nav = p.Collision.NavMesh;
            if (!nav) {
                ImGui::TextDisabled("No per-object navmesh in this asset.");
                ImGui::TextDisabled("(Object has no walkable nav cells; e.g. purely decorative.)");
                if (!p.Collision.CollisionMeshPath.empty())
                    ImGui::Text("Expected .bms: %s", p.Collision.CollisionMeshPath.c_str());
            } else {
                const auto kind = sro::nav::ClassifyPlacementNav(*nav);
                const auto edgeStats = sro::nav::CountBmsEdges(*nav);
                ImGui::Text("Nav kind: %s", sro::nav::BmsObjectNavKindLabel(kind));
                if (kind == sro::nav::BmsObjectNavKind::Building) {
                    ImGui::TextColored(ImVec4(1.f, 0.6f, 0.3f, 1.f),
                        "NOT walkable — all %d outline edges fully blocked (object nav volume only).",
                        edgeStats.outlineBlocked);
                } else if (kind == sro::nav::BmsObjectNavKind::WalkablePlatform) {
                    ImGui::TextColored(ImVec4(0.4f, 1.f, 0.5f, 1.f),
                        "Walkable platform — %d open terrain-facing / %d blocked outline edges.",
                        edgeStats.outlineOpenTerrain, edgeStats.outlineBlocked);
                }
                ImGui::Text("CellTri: %d cells (Flag is event data, not walkability)",
                    (int)nav->Cells.size());
                ImGui::Text("NavFlag: 0x%08X", nav->NavFlag);
                ImGui::Text("NavVertices: %d", (int)nav->Vertices.size());
                ImGui::Text("Outline edges: %d (%d terrain-facing open, %d blocked)",
                    edgeStats.outlineTotal, edgeStats.outlineOpenTerrain, edgeStats.outlineBlocked);
                ImGui::Text("Inline edges (internal): %d", edgeStats.inlineTotal);
                ImGui::Text("Events: %d", (int)nav->Events.size());
                ImGui::Text("Outline grid: %u x %u (%u cells)",
                            nav->OutlineGrid.Width, nav->OutlineGrid.Height, nav->OutlineGrid.CellCount);

                sro::formats::NavMesh* nvm = nullptr;
                if (ctx.sroRegionManager)
                    nvm = ctx.sroRegionManager->GetNavMesh(p.LoadedRx, p.LoadedRy);
                const auto diag = sro::nav::BuildPlacementDiagnostics(nvm, p, p.LoadedRx, p.LoadedRy);
                if (ImGui::TreeNode("Nav diagnostics")) {
                    ImGui::Text("NVM ObjectList: %s (index %d)",
                                diag.inNvm ? "yes" : "no", diag.nvmObjectIndex);
                    if (!diag.cellQuadIndices.empty()) {
                        ImGui::Text("CellQuad indices:");
                        for (int ci : diag.cellQuadIndices)
                            ImGui::Text("  QuadCell[%d]", ci);
                    }
                    ImGui::Text("Terrain-facing edges: %d open / %d blocked",
                                diag.terrainFacingOpen, diag.terrainFacingBlocked);
                    if (diag.navFlag != 0)
                        ImGui::Text("NavFlag: 0x%08X (Edge=%d Cell=%d Event=%d)",
                                    diag.navFlag,
                                    (diag.navFlag & 1) ? 1 : 0,
                                    (diag.navFlag & 2) ? 1 : 0,
                                    (diag.navFlag & 4) ? 1 : 0);
                    if (diag.eventCount > 0)
                        ImGui::Text("BMS events: %d", diag.eventCount);
                    if (nvm) {
                        const auto audit = sro::nav::AuditBmsNavUsage(*nav, nav->NavFlag);
                        for (const auto& w : audit)
                            ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "%s", w.c_str());
                    }
                    ImGui::TreePop();
                }

                if (ImGui::Button("Show on map")) {
                    Ui::ApplyBlockingViewPreset(ctx);
                    ctx.navmeshEditor.activeTab = NavMeshEditorTab::Analyze;
                    ctx.panels.navLayersPanel = true;
                    st.showCollisionBoxes = true;
                    if (ctx.sroRegionManager) {
                        const int rx = ctx.sroRegionManager->GetCurrentRx();
                        const int ry = ctx.sroRegionManager->GetCurrentRy();
                        Ui::FocusTileMapOnPlacement(ctx, p, rx, ry);
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Combined passability map, orange/green object nav, red edges, zoom to footprint.");

                if (!nav->Cells.empty() && ImGui::TreeNode("First cells (preview)")) {
                    int shown = std::min<int>(8, (int)nav->Cells.size());
                    for (int i = 0; i < shown; ++i) {
                        const auto& c = nav->Cells[i];
                        ImGui::Text("  Cell[%d] V=%u,%u,%u Flag=0x%04X (event bits, not walkable)",
                            i, c.V0, c.V1, c.V2, c.Flag);
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::Unindent(8.0f);
        }

        if (ImGui::CollapsingHeader("Authored collisionBox0 (raw)")) {
            ImGui::Indent(8.0f);
            if (!p.Collision.HasAuthoredBox) {
                ImGui::TextDisabled("BSR did not provide collisionBox0/1.");
            } else {
                for (int i = 0; i < 24; ++i) {
                    ImGui::PushID(i);
                    ImGui::SetNextItemWidth(70);
                    float v = p.Collision.Box0[i];
                    char lbl[16]; snprintf(lbl, sizeof(lbl), "[%d]", i);
                    ImGui::DragFloat(lbl, &v, 0.0f, 0.0f, 0.0f, "%.3f");
                    if ((i + 1) % 6 != 0) ImGui::SameLine();
                    ImGui::PopID();
                }
            }
            ImGui::Unindent(8.0f);
        }
    } else {
        ImGui::TextDisabled("Select a placement to inspect its collision resource chain.");
    }
    ImGui::EndChild();
}

static void DrawSettingsTab(EditorContext& ctx) {
    auto& st = ctx.collisionEditor;

    if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(8.0f);
        ImGui::Checkbox("Show BMS collision in viewport", &st.showCollisionBoxes);
        ImGui::TextDisabled("Layer toggles: see Nav Layers panel.");
        if (ImGui::Button("Open Nav Layers")) ctx.panels.navLayersPanel = true;
        ImGui::Checkbox("Show Only Selected", &st.showOnlySelected);
        ImGui::Unindent(8.0f);
    }
}

void Ui::DrawCollisionEditorPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Object Nav Inspector", &ctx.panels.collisionEditor)) { ImGui::End(); return; }

    if (ctx.sroRegionManager) {
        int rx = ctx.sroRegionManager->GetCurrentRx();
        int ry = ctx.sroRegionManager->GetCurrentRy();
        ImGui::Text("Region: (%d, %d)", rx, ry);
        ImGui::SameLine();
        if (ctx.sroPlacements) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d placements", (int)ctx.sroPlacements->size());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No placements");
        }
    } else {
        ImGui::TextDisabled("No region manager (client not loaded)");
    }

    ImGui::Separator();

    static int tab = 0;
    const char* tabNames[] = {"Overview", "Placements", "Settings"};
    for (int i = 0; i < IM_ARRAYSIZE(tabNames); ++i) {
        if (i > 0) ImGui::SameLine();
        bool active = (tab == i);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(tabNames[i])) tab = i;
        if (active) ImGui::PopStyleColor();
    }

    ImGui::Separator();

    switch (tab) {
    case 0: DrawOverviewTab(ctx); break;
    case 1: DrawPlacementListTab(ctx); break;
    case 2: DrawSettingsTab(ctx); break;
    }

    ImGui::End();
}
