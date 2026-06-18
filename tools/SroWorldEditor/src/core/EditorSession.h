#pragma once
#include "core/MathTypes.h"
#include "EditorAppTypes.h"
#include <string>

class EditorContext;
class EditorViewport;

struct EditorSession {
    std::string clientPath;
    int sroRegionRx = 97;
    int sroRegionRy = 167;
    Vector3 cameraPosition{960.0f, 150.0f, 960.0f};
    float cameraYaw = -90.0f;
    float cameraPitch = -30.0f;
    float cameraSpeed = 250.0f;
    bool valid = false;

    PanelVisibility panels{};
    ViewportSettings viewport{};
    NavLayerState navLayers{};
    EditorToolType activeTool = EditorToolType::Select;
    int objectViewerFilterTab = 0;
    std::string objectViewerSearch;
    bool objectViewerFollowSelection = true;
    bool objectViewerPreviewWireframe = false;
    bool objectViewerPreviewEffects = true;
    float objectViewerSplitCatalogPreview = 0.52f;
    float objectViewerSplitCatalogInspect = 0.45f;
    bool hasUiState = false;

    static std::wstring SessionFilePath();
    static std::wstring IniFilePath();
    static bool Load(EditorSession& out);
    static bool Save(const EditorSession& session);

    static void Capture(const EditorSession& src, EditorSession& dst);
    static void CaptureFrom(EditorContext& ctx, EditorViewport& viewport, EditorSession& out);
    static void ApplyUiToContext(EditorContext& ctx, const EditorSession& session);
};
