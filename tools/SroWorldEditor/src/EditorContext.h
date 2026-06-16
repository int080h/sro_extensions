#pragma once
#include "EditorAppTypes.h"
#include "project/Project.h"
#include "world/Region.h"
#include "validation/ValidationMessage.h"
#include "validation/Validator.h"
#include "core/CommandHistory.h"
#include "PlacementVM.h"
#include <optional>
#include <vector>
#include <string>

class MeshRenderer;
class RenderManager;
class RegionManager;

struct EditorContext {
    Project project;
    World world;
    std::optional<SelectionId> selection;
    int activeRegionId = 25000;
    EditorToolType activeTool = EditorToolType::Select;
    PanelVisibility panels;
    ViewportSettings viewport;
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
};
