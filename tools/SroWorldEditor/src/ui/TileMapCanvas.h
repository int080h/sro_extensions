#pragma once
#include "formats/NavMeshFormat.h"
#include "EditorContext.h"
#include "ui/NavLayerUi.h"
#include "imgui.h"

namespace Ui {

struct TileMapCanvasResult {
    bool selectionChanged = false;
    bool tilesModified = false;
    int hoverTx = -1;
    int hoverTz = -1;
};

inline bool IsTileOob(uint16_t flags) { return flags == 0xFFFF; }
inline bool IsTileBlocked(uint16_t flags) { return !IsTileOob(flags) && (flags & 0x0001) != 0; }
inline bool IsTileWalkable(uint16_t flags) { return !IsTileOob(flags) && (flags & 0x0001) == 0; }

const char* TileFlagLabel(uint16_t flags);
void FocusTileMapOnPlacement(EditorContext& ctx, const PlacementVM& p, int rx, int ry);

TileMapCanvasResult DrawTileMapCanvas(EditorContext& ctx, sro::formats::NavMesh& nav, bool allowPaint = true);

} // namespace Ui
