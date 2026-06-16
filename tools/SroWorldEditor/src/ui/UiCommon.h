#pragma once
#include "EditorContext.h"

class Editor;

namespace Ui {

void DrawMainMenuBar(EditorContext& ctx, ::Editor& editor);
void DrawToolbar(EditorContext& ctx, ::Editor& editor);
void DrawStatusBar(const EditorContext& ctx);
void DrawViewportPanel(EditorContext& ctx, ::Editor& editor);
void DrawWorldOutlinerPanel(EditorContext& ctx);
void DrawPropertiesPanel(EditorContext& ctx);
void DrawAssetBrowserPanel(EditorContext& ctx);
void DrawRegionManagerPanel(EditorContext& ctx, ::Editor& editor);
void DrawConsolePanel(EditorContext& ctx);
void DrawValidationPanel(EditorContext& ctx);
void DrawPerformancePanel(const EditorContext& ctx);
void DrawWorldMapPanel(EditorContext& ctx, ::Editor& editor);
void SetupDefaultDockLayout(unsigned int dockspaceId);

}
