#include "core/EditorSession.h"
#include "EditorContext.h"
#include "EditorViewport.h"
#include "EditorCamera.h"
#include "ui/NavLayerUi.h"
#include "core/FileSystem.h"
#include "core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <shlobj.h>

using json = nlohmann::json;

namespace {

std::wstring AppDataDir() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
        return L"";
    std::wstring base(path);
    CoTaskMemFree(path);
    std::filesystem::path dir = std::filesystem::path(base) / L"SroWorldEditor";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.wstring();
}

json PanelVisibilityToJson(const PanelVisibility& p) {
    return {
        {"viewport", p.viewport},
        {"worldOutliner", p.worldOutliner},
        {"properties", p.properties},
        {"assetBrowser", p.assetBrowser},
        {"objectViewer", p.objectViewer},
        {"regionManager", p.regionManager},
        {"worldMap", p.worldMap},
        {"console", p.console},
        {"validation", p.validation},
        {"performance", p.performance},
        {"spawnEditor", p.spawnEditor},
        {"npcEditor", p.npcEditor},
        {"teleportEditor", p.teleportEditor},
        {"zoneEditor", p.zoneEditor},
        {"collisionViewer", p.collisionViewer},
        {"navmeshEditor", p.navmeshEditor},
        {"terrainNavPanel", p.terrainNavPanel},
        {"navLayersPanel", p.navLayersPanel},
        {"aiNavDataPanel", p.aiNavDataPanel},
        {"dungeonNavPanel", p.dungeonNavPanel},
        {"navMeshBrowser", p.navMeshBrowser},
        {"collisionEditor", p.collisionEditor},
    };
}

void PanelVisibilityFromJson(const json& j, PanelVisibility& p) {
    if (!j.is_object()) return;
    p.viewport = j.value("viewport", p.viewport);
    p.worldOutliner = j.value("worldOutliner", p.worldOutliner);
    p.properties = j.value("properties", p.properties);
    p.assetBrowser = j.value("assetBrowser", p.assetBrowser);
    p.objectViewer = j.value("objectViewer", p.objectViewer);
    p.regionManager = j.value("regionManager", p.regionManager);
    p.worldMap = j.value("worldMap", p.worldMap);
    p.console = j.value("console", p.console);
    p.validation = j.value("validation", p.validation);
    p.performance = j.value("performance", p.performance);
    p.spawnEditor = j.value("spawnEditor", p.spawnEditor);
    p.npcEditor = j.value("npcEditor", p.npcEditor);
    p.teleportEditor = j.value("teleportEditor", p.teleportEditor);
    p.zoneEditor = j.value("zoneEditor", p.zoneEditor);
    p.collisionViewer = j.value("collisionViewer", p.collisionViewer);
    p.navmeshEditor = j.value("navmeshEditor", p.navmeshEditor);
    p.terrainNavPanel = j.value("terrainNavPanel", p.terrainNavPanel);
    if (p.navmeshEditor && !p.terrainNavPanel)
        p.terrainNavPanel = true;
    p.aiNavDataPanel = j.value("aiNavDataPanel", p.aiNavDataPanel);
    p.dungeonNavPanel = j.value("dungeonNavPanel", p.dungeonNavPanel);
    p.navMeshBrowser = j.value("navMeshBrowser", p.navMeshBrowser);
    p.navLayersPanel = j.value("navLayersPanel", p.navLayersPanel);
    p.collisionEditor = j.value("collisionEditor", p.collisionEditor);
}

json ViewportSettingsToJson(const ViewportSettings& v) {
    return {
        {"showGrid", v.showGrid},
        {"showRegionBounds", v.showRegionBounds},
        {"showCollision", v.showCollision},
        {"showSpawns", v.showSpawns},
        {"showNpcs", v.showNpcs},
        {"showTeleports", v.showTeleports},
        {"showEventDecors", v.showEventDecors},
        {"showParticles", v.showParticles},
        {"snapToGrid", v.snapToGrid},
        {"snapToGround", v.snapToGround},
        {"localTransform", v.localTransform},
        {"cameraSpeed", v.cameraSpeed},
        {"viewMode", static_cast<int>(v.viewMode)},
    };
}

void ViewportSettingsFromJson(const json& j, ViewportSettings& v) {
    if (!j.is_object()) return;
    v.showGrid = j.value("showGrid", v.showGrid);
    v.showRegionBounds = j.value("showRegionBounds", v.showRegionBounds);
    v.showCollision = j.value("showCollision", v.showCollision);
    v.showSpawns = j.value("showSpawns", v.showSpawns);
    v.showNpcs = j.value("showNpcs", v.showNpcs);
    v.showTeleports = j.value("showTeleports", v.showTeleports);
    v.showEventDecors = j.value("showEventDecors", v.showEventDecors);
    v.showParticles = j.value("showParticles", v.showParticles);
    v.snapToGrid = j.value("snapToGrid", v.snapToGrid);
    v.snapToGround = j.value("snapToGround", v.snapToGround);
    v.localTransform = j.value("localTransform", v.localTransform);
    v.cameraSpeed = j.value("cameraSpeed", v.cameraSpeed);
    v.viewMode = static_cast<ViewportMode>(j.value("viewMode", static_cast<int>(v.viewMode)));
}

json NavLayerStateToJson(const NavLayerState& L) {
    return {
        {"showTerrainBlocked", L.showTerrainBlocked},
        {"showTerrainWalkable", L.showTerrainWalkable},
        {"showTileMapOverlay", L.showTileMapOverlay},
        {"showHeightMapSurface", L.showHeightMapSurface},
        {"showHeightMapWireframe", L.showHeightMapWireframe},
        {"showPlaneMap", L.showPlaneMap},
        {"showPlaneMapOnTileMap", L.showPlaneMapOnTileMap},
        {"showCellQuads", L.showCellQuads},
        {"showInternalEdges", L.showInternalEdges},
        {"showGlobalEdges", L.showGlobalEdges},
        {"showBmsWikiCollision", L.showBmsWikiCollision},
        {"showBmsPassabilityClass", L.showBmsPassabilityClass},
        {"showBmsBuilding", L.showBmsBuilding},
        {"showBmsPlatform", L.showBmsPlatform},
        {"showBmsEdgeDebug", L.showBmsEdgeDebug},
        {"showLinkEdges", L.showLinkEdges},
        {"hideWorldGeometry", L.hideWorldGeometry},
        {"showNeighborRegions", L.showNeighborRegions},
        {"navTopDownCamera", L.navTopDownCamera},
        {"showDofBlocks", L.showDofBlocks},
        {"showDofVoxels", L.showDofVoxels},
        {"showDofLinks", L.showDofLinks},
        {"showDofObjects", L.showDofObjects},
    };
}

void NavLayerStateFromJson(const json& j, NavLayerState& L) {
    if (!j.is_object()) return;
    L.showTerrainBlocked = j.value("showTerrainBlocked", L.showTerrainBlocked);
    L.showTerrainWalkable = j.value("showTerrainWalkable", L.showTerrainWalkable);
    L.showTileMapOverlay = j.value("showTileMapOverlay", L.showTileMapOverlay);
    L.showHeightMapSurface = j.value("showHeightMapSurface", L.showHeightMapSurface);
    L.showHeightMapWireframe = j.value("showHeightMapWireframe", L.showHeightMapWireframe);
    L.showPlaneMap = j.value("showPlaneMap", L.showPlaneMap);
    L.showPlaneMapOnTileMap = j.value("showPlaneMapOnTileMap", L.showPlaneMapOnTileMap);
    L.showCellQuads = j.value("showCellQuads", L.showCellQuads);
    L.showInternalEdges = j.value("showInternalEdges", L.showInternalEdges);
    L.showGlobalEdges = j.value("showGlobalEdges", L.showGlobalEdges);
    L.showBmsWikiCollision = j.value("showBmsWikiCollision", L.showBmsWikiCollision);
    L.showBmsPassabilityClass = j.value("showBmsPassabilityClass", L.showBmsPassabilityClass);
    L.showBmsBuilding = j.value("showBmsBuilding", L.showBmsBuilding);
    L.showBmsPlatform = j.value("showBmsPlatform", L.showBmsPlatform);
    L.showBmsEdgeDebug = j.value("showBmsEdgeDebug", L.showBmsEdgeDebug);
    L.showLinkEdges = j.value("showLinkEdges", L.showLinkEdges);
    L.hideWorldGeometry = j.value("hideWorldGeometry", L.hideWorldGeometry);
    L.showNeighborRegions = j.value("showNeighborRegions", L.showNeighborRegions);
    L.navTopDownCamera = j.value("navTopDownCamera", L.navTopDownCamera);
    L.showDofBlocks = j.value("showDofBlocks", L.showDofBlocks);
    L.showDofVoxels = j.value("showDofVoxels", L.showDofVoxels);
    L.showDofLinks = j.value("showDofLinks", L.showDofLinks);
    L.showDofObjects = j.value("showDofObjects", L.showDofObjects);
}

} // namespace

std::wstring EditorSession::SessionFilePath() {
    const std::wstring dir = AppDataDir();
    return dir.empty() ? L"" : dir + L"/session.json";
}

std::wstring EditorSession::IniFilePath() {
    const std::wstring dir = AppDataDir();
    return dir.empty() ? L"" : dir + L"/imgui.ini";
}

bool EditorSession::Load(EditorSession& out) {
    const std::wstring path = SessionFilePath();
    if (path.empty() || !FileExists(path))
        return false;

    std::ifstream in(path);
    if (!in) return false;

    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }

    out.clientPath = j.value("clientPath", "");
    out.sroRegionRx = j.value("sroRegionRx", 97);
    out.sroRegionRy = j.value("sroRegionRy", 167);
    out.cameraYaw = j.value("cameraYaw", -90.0f);
    out.cameraPitch = j.value("cameraPitch", -30.0f);
    out.cameraSpeed = j.value("cameraSpeed", 250.0f);
    out.valid = j.value("valid", false);
    if (j.contains("cameraPosition")) {
        out.cameraPosition.x = j["cameraPosition"].value("x", 960.0f);
        out.cameraPosition.y = j["cameraPosition"].value("y", 150.0f);
        out.cameraPosition.z = j["cameraPosition"].value("z", 960.0f);
    }

    out.hasUiState = false;
    if (j.contains("ui") && j["ui"].is_object()) {
        const json& ui = j["ui"];
        out.hasUiState = true;
        if (ui.contains("panels")) PanelVisibilityFromJson(ui["panels"], out.panels);
        if (ui.contains("viewport")) ViewportSettingsFromJson(ui["viewport"], out.viewport);
        if (ui.contains("navLayers")) NavLayerStateFromJson(ui["navLayers"], out.navLayers);
        out.activeTool = static_cast<EditorToolType>(ui.value("activeTool", static_cast<int>(out.activeTool)));
        if (ui.contains("objectViewer") && ui["objectViewer"].is_object()) {
            const json& ov = ui["objectViewer"];
            out.objectViewerFilterTab = ov.value("filterTab", 0);
            out.objectViewerSearch = ov.value("search", "");
            out.objectViewerFollowSelection = ov.value("followSelection", true);
            out.objectViewerPreviewWireframe = ov.value("previewWireframe", false);
            out.objectViewerPreviewEffects = ov.value("previewEffects", true);
            out.objectViewerSplitCatalogPreview = ov.value("splitCatalogPreview", 0.52f);
            out.objectViewerSplitCatalogInspect = ov.value("splitCatalogInspect", 0.45f);
        }
    }
    return true;
}

bool EditorSession::Save(const EditorSession& session) {
    const std::wstring path = SessionFilePath();
    if (path.empty()) return false;

    json j;
    j["clientPath"] = session.clientPath;
    j["sroRegionRx"] = session.sroRegionRx;
    j["sroRegionRy"] = session.sroRegionRy;
    j["cameraYaw"] = session.cameraYaw;
    j["cameraPitch"] = session.cameraPitch;
    j["cameraSpeed"] = session.cameraSpeed;
    j["valid"] = session.valid;
    j["cameraPosition"] = {{"x", session.cameraPosition.x}, {"y", session.cameraPosition.y}, {"z", session.cameraPosition.z}};
    j["ui"] = {
        {"panels", PanelVisibilityToJson(session.panels)},
        {"viewport", ViewportSettingsToJson(session.viewport)},
        {"navLayers", NavLayerStateToJson(session.navLayers)},
        {"activeTool", static_cast<int>(session.activeTool)},
        {"objectViewer", {
            {"filterTab", session.objectViewerFilterTab},
            {"search", session.objectViewerSearch},
            {"followSelection", session.objectViewerFollowSelection},
            {"previewWireframe", session.objectViewerPreviewWireframe},
            {"previewEffects", session.objectViewerPreviewEffects},
            {"splitCatalogPreview", session.objectViewerSplitCatalogPreview},
            {"splitCatalogInspect", session.objectViewerSplitCatalogInspect},
        }},
    };

    std::ofstream out(path);
    if (!out) return false;
    out << j.dump(2);
    return true;
}

void EditorSession::Capture(const EditorSession& src, EditorSession& dst) {
    dst = src;
}

void EditorSession::CaptureFrom(EditorContext& ctx, EditorViewport& viewport, EditorSession& out) {
    out.clientPath = ctx.project.clientPath;
    out.sroRegionRx = ctx.sroRegionRx;
    out.sroRegionRy = ctx.sroRegionRy;
    out.cameraPosition = viewport.GetCamera().Position;
    out.cameraYaw = viewport.GetCamera().Yaw;
    out.cameraPitch = viewport.GetCamera().Pitch;
    out.cameraSpeed = ctx.viewport.cameraSpeed;
    out.valid = ctx.sroClientLoaded && !out.clientPath.empty();

    out.panels = ctx.panels;
    out.viewport = ctx.viewport;
    out.navLayers = ctx.navLayers;
    out.activeTool = ctx.activeTool;
    out.objectViewerFilterTab = ctx.objectViewer.filterTab;
    out.objectViewerSearch = ctx.objectViewer.catalogSearch;
    out.objectViewerFollowSelection = ctx.objectViewer.followSelection;
    out.objectViewerPreviewWireframe = ctx.objectViewer.previewWireframe;
    out.objectViewerPreviewEffects = ctx.objectViewer.previewEffects;
    out.objectViewerSplitCatalogPreview = ctx.objectViewer.splitCatalogPreview;
    out.objectViewerSplitCatalogInspect = ctx.objectViewer.splitCatalogInspect;
    out.hasUiState = true;
}

void EditorSession::ApplyUiToContext(EditorContext& ctx, const EditorSession& session) {
    if (!session.hasUiState) return;
    ctx.panels = session.panels;
    ctx.viewport = session.viewport;
    ctx.navLayers = session.navLayers;
    Ui::SyncNavLayersFromLegacy(ctx);
    ctx.activeTool = session.activeTool;
    ctx.objectViewer.filterTab = session.objectViewerFilterTab;
    ctx.objectViewer.catalogSearch = session.objectViewerSearch;
    ctx.objectViewer.followSelection = session.objectViewerFollowSelection;
    ctx.objectViewer.previewWireframe = session.objectViewerPreviewWireframe;
    ctx.objectViewer.previewEffects = session.objectViewerPreviewEffects;
    ctx.objectViewer.splitCatalogPreview = session.objectViewerSplitCatalogPreview;
    ctx.objectViewer.splitCatalogInspect = session.objectViewerSplitCatalogInspect;
    ctx.objectViewer.filterTabRestorePending = true;
}
