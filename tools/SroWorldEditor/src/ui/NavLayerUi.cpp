#include "ui/NavLayerUi.h"
#include "EditorPublic.h"
#include "imgui.h"

namespace Ui {

void SyncNavLayersFromLegacy(EditorContext& ctx) {
    auto& layers = ctx.navLayers;
    auto& st = ctx.navmeshEditor;
    if (st.showBlocked) layers.showTerrainBlocked = true;
    if (st.showWalkable) layers.showTerrainWalkable = true;
    if (st.showCells) layers.showCellQuads = true;
    if (st.showEdges) {
        layers.showInternalEdges = true;
        layers.showGlobalEdges = true;
    }
    if (st.showObjectNavSurface) {
        layers.showBmsPassabilityClass = true;
        layers.showBmsBuilding = true;
        layers.showBmsPlatform = true;
    }
    if (st.showObjectNavEdges) layers.showBmsEdgeDebug = true;
    if (st.showLinkEdges) layers.showLinkEdges = true;
    if (st.showNeighborRegions) layers.showNeighborRegions = true;
    if (st.navHideWorldGeometry) layers.hideWorldGeometry = true;
    if (st.navTopDownCamera) layers.navTopDownCamera = true;
    layers.showDofBlocks = st.showDofBlocks;
    layers.showDofVoxels = st.showDofVoxels;
    layers.showDofLinks = st.showDofLinks;
    layers.showDofObjects = st.showDofObjects;
}

void ResetNavLayers(EditorContext& ctx) {
    ctx.navLayers = NavLayerState{};
    ctx.navmeshEditor.blockingViewActive = false;
}

void ApplyBlockingViewPreset(EditorContext& ctx) {
    ResetNavLayers(ctx);
    auto& L = ctx.navLayers;
    L.showTerrainBlocked = true;
    L.showBmsWikiCollision = true;
    L.showInternalEdges = true;
    L.hideWorldGeometry = true;
    L.navTopDownCamera = true;
    ctx.navmeshEditor.tileColorMode = 4;
    ctx.navmeshEditor.blockingViewActive = true;
    ctx.viewport.viewMode = ViewportMode::NavEdit;
    ctx.panels.terrainNavPanel = true;
    ctx.panels.navLayersPanel = true;
}

void ApplyTerrainDataPreset(EditorContext& ctx) {
    ResetNavLayers(ctx);
    auto& L = ctx.navLayers;
    L.showTileMapOverlay = true;
    L.showHeightMapWireframe = true;
    L.showPlaneMap = true;
    ctx.navmeshEditor.tileColorMode = 0;
}

void ApplyPassabilityPreset(EditorContext& ctx) {
    ResetNavLayers(ctx);
    auto& L = ctx.navLayers;
    L.showTerrainBlocked = true;
    L.showBmsPassabilityClass = true;
    L.showBmsBuilding = true;
    L.showBmsPlatform = true;
    ctx.navmeshEditor.tileColorMode = 4;
}

void ApplyFullGraphPreset(EditorContext& ctx) {
    ResetNavLayers(ctx);
    auto& L = ctx.navLayers;
    L.showTerrainBlocked = true;
    L.showTerrainWalkable = true;
    L.showTileMapOverlay = true;
    L.showCellQuads = true;
    L.showInternalEdges = true;
    L.showGlobalEdges = true;
    L.showBmsWikiCollision = true;
    L.showBmsPassabilityClass = true;
    L.showBmsEdgeDebug = true;
    L.showLinkEdges = true;
}

void DrawNavColorLegend() {
    ImGui::TextDisabled("Legend:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.35f, 0.35f, 1.f), "Red");
    ImGui::SameLine(); ImGui::Text("= blocked");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.55f, 0.4f, 1.f), "Rose");
    ImGui::SameLine(); ImGui::Text("= BMS walkable");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.f, 0.55f, 0.15f, 1.f), "Orange");
    ImGui::SameLine(); ImGui::Text("= building");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.35f, 1.f), "Green");
    ImGui::SameLine(); ImGui::Text("= platform");
}

void DrawNavPresetsRow(EditorContext& ctx) {
    if (ImGui::Button("Blocking view")) ApplyBlockingViewPreset(ctx);
    ImGui::SameLine();
    if (ImGui::Button("Terrain data")) ApplyTerrainDataPreset(ctx);
    ImGui::SameLine();
    if (ImGui::Button("Passability")) ApplyPassabilityPreset(ctx);
    ImGui::SameLine();
    if (ImGui::Button("Full graph")) ApplyFullGraphPreset(ctx);
    ImGui::SameLine();
    if (ImGui::Button("Reset layers")) ResetNavLayers(ctx);
}

} // namespace Ui
