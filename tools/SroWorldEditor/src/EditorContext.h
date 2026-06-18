#pragma once
#include "EditorAppTypes.h"
#include "project/Project.h"
#include "world/Region.h"
#include "validation/ValidationMessage.h"
#include "validation/Validator.h"
#include "core/CommandHistory.h"
#include "PlacementVM.h"
#include "formats/DofFormat.h"
#include <functional>
#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace sro::nav { class NavMeshSession; }

class MeshRenderer;
class EffectRenderer;
class RenderManager;
class RegionManager;

namespace sro {
class AssetResolver;
class TextDataCatalog;
}

struct EditorContext {
    Project project;
    World world;
    std::optional<SelectionId> selection;
    int activeRegionId = 25000;
    EditorToolType activeTool = EditorToolType::Select;
    PanelVisibility panels;
    ViewportSettings viewport;
    ObjectViewerState objectViewer;
    NpcEditorState npcEditorState;
    NavMeshEditorState navmeshEditor;
    NavLayerState navLayers;
    AiNavDataEditorState aiNavDataEditor;
    NavMeshBrowserState navMeshBrowser;
    CollisionEditorState collisionEditor;
    CommandHistory commandHistory;
    std::vector<ValidationMessage> validationMessages;
    Vector3 mouseWorldPos{};
    float fps = 0.0f;
    std::string selectedObjectName;
    bool sroClientLoaded = false;
    int sroRegionRx = 97;
    int sroRegionRy = 167;

    std::vector<PlacementVM>* sroPlacements = nullptr;
    RegionManager* sroRegionManager = nullptr;
    RenderManager* sroRenderManager = nullptr;
    MeshRenderer* sroMeshRenderer = nullptr;
    EffectRenderer* sroEffectRenderer = nullptr;
    const sro::AssetResolver* sroAssets = nullptr;
    const sro::TextDataCatalog* sroTextData = nullptr;
    sro::nav::NavMeshSession* navMeshSession = nullptr;
    std::function<void(PlacementVM&)> ensurePlacementCollision;

    // Loaded dungeon (JMXVDOF / RTNavMeshDungeon) for inspection + rendering.
    std::string loadedDofPath;
    std::shared_ptr<sro::formats::DofDungeon> loadedDof;
    int selectedDofBlockIdx = -1;

    static int EncodeRegionId(int rx, int ry) { return (rx << 8) | ry; }
    static void DecodeRegionId(int regionId, int& rx, int& ry) {
        rx = (regionId >> 8) & 0xFF;
        ry = regionId & 0xFF;
    }

    void Select(const SelectionId& id);
    void ClearSelection();
    void MarkModified();
    void RefreshValidation();
    std::string SelectionDisplayName() const;
    PlacementVM* FindPlacement(int16_t uid, int rx, int ry);
    PlacementVM* FindPlacementBySelection();
    void SyncObjectViewerFromSelection();
    void InspectGameObject(const std::string& codeName);
    void InspectBsrPath(const std::string& bsrPath);
    void InspectObjId(uint32_t objId);
};
