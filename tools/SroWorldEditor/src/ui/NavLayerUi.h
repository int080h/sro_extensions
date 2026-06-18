#pragma once

struct EditorContext;

namespace Ui {

void SyncNavLayersFromLegacy(EditorContext& ctx);
void ApplyBlockingViewPreset(EditorContext& ctx);
void ApplyTerrainDataPreset(EditorContext& ctx);
void ApplyPassabilityPreset(EditorContext& ctx);
void ApplyFullGraphPreset(EditorContext& ctx);
void ResetNavLayers(EditorContext& ctx);

void DrawNavColorLegend();
void DrawNavPresetsRow(EditorContext& ctx);

} // namespace Ui
