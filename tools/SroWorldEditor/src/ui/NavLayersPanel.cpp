#include "ui/NavLayersPanel.h"
#include "ui/NavLayerUi.h"
#include "EditorPublic.h"
#include "imgui.h"

namespace Ui {

static bool LayerToggle(const char* label, bool* v, ImU32 swatch) {
    ImGui::PushID(label);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + 14, p.y + 14), swatch);
    ImGui::Dummy(ImVec2(18, 14));
    ImGui::SameLine();
    const bool changed = ImGui::Checkbox(label, v);
    ImGui::PopID();
    return changed;
}

void DrawNavLayersPanel(EditorContext& ctx) {
    if (!ImGui::Begin("Nav Layers", &ctx.panels.navLayersPanel)) {
        ImGui::End();
        return;
    }

    DrawNavColorLegend();
    ImGui::Separator();
    DrawNavPresetsRow(ctx);
    ImGui::Separator();

    auto& L = ctx.navLayers;

    if (ImGui::CollapsingHeader("Terrain — TileMap", ImGuiTreeNodeFlags_DefaultOpen)) {
        LayerToggle("Terrain tile blocked", &L.showTerrainBlocked, IM_COL32(230, 45, 45, 255));
        LayerToggle("Terrain corridor grid", &L.showTerrainWalkable, IM_COL32(220, 220, 220, 200));
        LayerToggle("3D tile overlay", &L.showTileMapOverlay, IM_COL32(100, 150, 220, 200));
    }
    if (ImGui::CollapsingHeader("Terrain — HeightMap")) {
        LayerToggle("Height surface", &L.showHeightMapSurface, IM_COL32(80, 160, 80, 180));
        LayerToggle("Height wireframe", &L.showHeightMapWireframe, IM_COL32(200, 200, 100, 200));
    }
    if (ImGui::CollapsingHeader("Terrain — PlaneMap")) {
        LayerToggle("Water/ice slabs (3D)", &L.showPlaneMap, IM_COL32(40, 100, 220, 180));
        LayerToggle("Plane grid on tile map (2D)", &L.showPlaneMapOnTileMap, IM_COL32(40, 100, 220, 120));
    }
    if (ImGui::CollapsingHeader("Terrain — Cell graph", ImGuiTreeNodeFlags_DefaultOpen)) {
        LayerToggle("CellQuad outlines", &L.showCellQuads, IM_COL32(255, 255, 255, 200));
        LayerToggle("Internal edges", &L.showInternalEdges, IM_COL32(60, 220, 80, 255));
        LayerToggle("Global edges", &L.showGlobalEdges, IM_COL32(255, 220, 60, 255));
    }
    if (ImGui::CollapsingHeader("Object — BMS", ImGuiTreeNodeFlags_DefaultOpen)) {
        LayerToggle("BMS collision (wiki)", &L.showBmsWikiCollision, IM_COL32(180, 100, 120, 200));
        LayerToggle("Passability class", &L.showBmsPassabilityClass, IM_COL32(230, 120, 30, 200));
        LayerToggle("Building cells", &L.showBmsBuilding, IM_COL32(230, 120, 30, 255));
        LayerToggle("Platform cells", &L.showBmsPlatform, IM_COL32(60, 200, 60, 255));
        LayerToggle("Edge debug (EdgeFlag)", &L.showBmsEdgeDebug, IM_COL32(255, 40, 40, 255));
        LayerToggle("LinkEdges", &L.showLinkEdges, IM_COL32(255, 0, 255, 220));
    }
    if (ImGui::CollapsingHeader("View")) {
        LayerToggle("Hide world geometry", &L.hideWorldGeometry, IM_COL32(80, 80, 80, 255));
        LayerToggle("Neighbor region frames", &L.showNeighborRegions, IM_COL32(180, 180, 255, 200));
        LayerToggle("Snap top-down camera", &L.navTopDownCamera, IM_COL32(200, 200, 200, 255));
    }
    if (ctx.loadedDof) {
        if (ImGui::CollapsingHeader("Dungeon (DOF)")) {
            LayerToggle("DOF blocks", &L.showDofBlocks, IM_COL32(200, 100, 50, 200));
            LayerToggle("DOF voxels", &L.showDofVoxels, IM_COL32(150, 150, 200, 200));
            LayerToggle("DOF links", &L.showDofLinks, IM_COL32(255, 200, 0, 200));
            LayerToggle("DOF objects", &L.showDofObjects, IM_COL32(100, 255, 100, 200));
        }
    }

    ImGui::End();
}

} // namespace Ui
