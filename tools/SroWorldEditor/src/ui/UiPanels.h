#pragma once
#include "EditorContext.h"

namespace Ui {

void DrawMainMenuBar(EditorContext& ctx, class Editor& editor);
void DrawToolbar(EditorContext& ctx, class Editor& editor);
void DrawStatusBar(const EditorContext& ctx);
void DrawViewportPanel(EditorContext& ctx, class Editor& editor);
void DrawWorldOutlinerPanel(EditorContext& ctx);
void DrawPropertiesPanel(EditorContext& ctx);
void DrawAssetBrowserPanel(EditorContext& ctx);
void DrawRegionManagerPanel(EditorContext& ctx, class Editor& editor);
void DrawConsolePanel(EditorContext& ctx);
void DrawValidationPanel(EditorContext& ctx);
void DrawPerformancePanel(const EditorContext& ctx);
void DrawTerrainNavPanel(EditorContext& ctx);
void DrawAiNavDataPanel(EditorContext& ctx);
void DrawDungeonNavPanel(EditorContext& ctx);
void DrawNavMeshBrowserPanel(EditorContext& ctx, class Editor& editor);
void DrawCollisionEditorPanel(EditorContext& ctx);

void SetupDefaultDockLayout(ImGuiID dockspaceId);

} // namespace Ui
