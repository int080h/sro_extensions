#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class EditorToolType {
    Select, Move, Rotate, Scale, Terrain,
    ObjectPlacement, SpawnPlacement, NpcPlacement, TeleportPlacement,
    ZonePaint, CollisionPaint, Measure
};

enum class ViewportMode {
    Lit, Wireframe, Collision, ZoneOverlay, SpawnOverlay, RegionDebug, NavEdit
};

enum class EntityKind {
    WorldObject, MapPlacement, Npc, SpawnPoint, TeleportPoint, Zone, Region
};

struct SelectionId {
    EntityKind kind = EntityKind::WorldObject;
    int regionId = 0;
    std::string id;
    bool operator==(const SelectionId& other) const {
        return kind == other.kind && regionId == other.regionId && id == other.id;
    }
};

struct PanelVisibility {
    bool viewport = true;
    bool worldOutliner = true;
    bool properties = true;
    bool assetBrowser = true;
    bool objectViewer = true;
    bool regionManager = false;
    bool worldMap = false;
    bool console = true;
    bool validation = true;
    bool performance = true;
    bool spawnEditor = false;
    bool npcEditor = false;
    bool teleportEditor = false;
    bool zoneEditor = false;
    bool collisionViewer = false;
    bool navmeshEditor = false;
    bool terrainNavPanel = false;
    bool navLayersPanel = true;
    bool aiNavDataPanel = false;
    bool dungeonNavPanel = false;
    bool navMeshBrowser = false;
    bool collisionEditor = false;
};

enum class ObjectInspectSource {
    None,
    GameObject,
    BsrPath,
    ObjId
};

struct ObjectViewerState {
    ObjectInspectSource source = ObjectInspectSource::None;
    std::string inspectedCodeName;
    std::string inspectedBsrPath;
    uint32_t inspectedObjId = 0;
    std::string selectedAnimPath;
    int selectedAnimIndex = -1;
    bool animPlaying = false;
    float animTimeMs = 0.f;
    float animSpeed = 1.f;
    int skinningDebugMode = 0;
    int banLocalMode = 0;
    bool bindOnlyFk = false;
    bool followSelection = true;
    bool previewWireframe = false;
    bool previewEffects = true;
    int filterTab = 0;
    std::string catalogSearch;
    float previewYaw = 0.8f;
    float previewPitch = 0.35f;
    float previewDistance = 6.f;
    float previewPanX = 0.f;
    float previewPanY = 0.f;
    bool previewDragActive = false;
    bool filterTabRestorePending = false;
    float splitCatalogPreview = 0.52f;
    float splitCatalogInspect = 0.45f;

    void ResetPreviewCamera() {
        previewYaw = 0.8f;
        previewPitch = 0.35f;
        previewDistance = 6.f;
        previewPanX = 0.f;
        previewPanY = 0.f;
    }
};

struct NpcEditorState {
    std::string newCodeName = "NPC_CH_SMITH";
    int selectedCatalogIndex = -1;
};

struct ViewportSettings {
    bool showGrid = true;
    bool showRegionBounds = true;
    bool showCollision = false;
    bool showSpawns = true;
    bool showNpcs = true;
    bool showTeleports = true;
    bool showEventDecors = false;
    bool showParticles = true;
    bool snapToGrid = false;
    bool snapToGround = true;
    bool localTransform = false;
    float cameraSpeed = 250.0f;
    ViewportMode viewMode = ViewportMode::Lit;
};

enum class NavMeshEditorTab {
    Analyze, TerrainData, CellGraph, NvmObjects
};

struct NavLayerState {
    bool showTerrainBlocked = true;
    bool showTerrainWalkable = false;
    bool showTileMapOverlay = false;
    bool showHeightMapSurface = false;
    bool showHeightMapWireframe = false;
    bool showPlaneMap = false;
    bool showPlaneMapOnTileMap = false;
    bool showCellQuads = false;
    bool showInternalEdges = false;
    bool showGlobalEdges = false;
    bool showBmsWikiCollision = true;
    bool showBmsPassabilityClass = false;
    bool showBmsBuilding = false;
    bool showBmsPlatform = false;
    bool showBmsEdgeDebug = false;
    bool showLinkEdges = false;
    bool hideWorldGeometry = false;
    bool showNeighborRegions = false;
    bool navTopDownCamera = false;
    bool showDofBlocks = true;
    bool showDofVoxels = false;
    bool showDofLinks = true;
    bool showDofObjects = true;
};

struct NavMeshEditorState {
    NavMeshEditorTab activeTab = NavMeshEditorTab::Analyze;
    int selectedObjectIdx = -1;
    int selectedCellIdx = -1;
    int selectedGlobalEdgeIdx = -1;
    int selectedInternalEdgeIdx = -1;
    int selectedTileIdx = -1;
    char objectSearch[128] = {};
    char cellSearch[64] = {};
    int tileColorMode = 0;
    float tileZoom = 6.0f;
    bool showWalkable = false;
    bool showBlocked = true;
    bool showEdges = false;
    bool showCells = false;
    bool showObjects = true;
    bool showHeightmap = false;
    bool showNavmeshBlockers = true;
    bool showLinkEdges = false;
    bool showObjectNavEdges = true;
    bool showObjectNavSurface = false;
    bool blockingViewActive = false;
    bool navHideWorldGeometry = true;
    bool showNeighborRegions = false;
    bool navTopDownCamera = false;
    float brushSize = 1.0f;
    int paintMode = 0;
    bool pathfindTestActive = false;
    int pathfindStartCell = -1;
    int pathfindEndCell = -1;
    std::vector<int> pathfindResult;
    bool followViewportSelection = true;
    // Dungeon (JMXVDOF) viewer
    char dofPath[512] = {};
    bool showDofBlocks = true;
    bool showDofVoxels = false;
    bool showDofLinks = true;
    bool showDofObjects = true;
};

struct AiNavDataEditorState {
    uint16_t selectedRegionId = 0;
    int selectedBlockIdx = -1;
    int selectedLinkIdx = -1;
    int selectedSimpleBlockIdx = -1;
    int selectedEdgeIdx = -1;
    int pathfindStartBlock = 0;
    int pathfindEndBlock = 0;
    std::vector<int> pathfindResult;
    bool followViewportRegion = true;
    int lastSyncedRx = -1;
    int lastSyncedRy = -1;
    int catalogPickerIdx = 0;
};

struct NavMeshBrowserState {
    char searchFilter[256] = {};
    int filterKind = 0;
    int selectedIndex = -1;
    bool showCreateTerrainPopup = false;
    bool showCreateAiNavPopup = false;
    int createRx = 97;
    int createRy = 167;
    int createRegionId = 0;
};

struct CollisionEditorState {
    int selectedPlacementIdx = -1;
    char searchFilter[128] = {};
    bool showCollisionBoxes = true;   // show real per-object navmesh (RTNavMeshObj)
    bool showOnlySelected = false;
    bool snapToGrid = false;
    float gridSize = 5.0f;
};
