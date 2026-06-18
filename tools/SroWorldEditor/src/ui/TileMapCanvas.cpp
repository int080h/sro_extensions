#include "ui/TileMapCanvas.h"
#include "ui/NavLayerUi.h"
#include "sro/nav/TileCoord.h"
#include "runtime/RegionManager.h"
#include "rendering/RenderManager.h"
#include "rendering/NvmRenderer.h"
#include "sro/nav/ObjectNavSpatial.h"
#include "core/EditorAction.h"
#include "ui/TileMapCanvas.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Ui {

namespace {
constexpr int kModeWalkableBlocked = 0;
constexpr int kModeCellId = 1;
constexpr int kModeTextureId = 2;
constexpr int kModeObjectBlockedFp = 3;
constexpr int kModeCombinedPassability = 4;
constexpr int kModeObjectCoverage = 5;

bool IsReadOnlyColorMode(int mode) {
    return mode == kModeObjectBlockedFp || mode == kModeCombinedPassability || mode == kModeObjectCoverage;
}

void EnsureRegionBmsLoaded(EditorContext& ctx, int rx, int ry) {
    if (!ctx.sroPlacements || !ctx.ensurePlacementCollision)
        return;
    for (auto& p : *ctx.sroPlacements) {
        if (p.LoadedRx == rx && p.LoadedRy == ry && !p.Collision.NavMesh)
            ctx.ensurePlacementCollision(p);
    }
}

ImU32 PassabilityKindColor(sro::nav::PassabilityKind kind) {
    switch (kind) {
    case sro::nav::PassabilityKind::Passable: return IM_COL32(40, 130, 55, 220);
    case sro::nav::PassabilityKind::TerrainBlocked: return IM_COL32(210, 45, 45, 255);
    case sro::nav::PassabilityKind::ObjectBuilding: return IM_COL32(255, 140, 30, 240);
    case sro::nav::PassabilityKind::ObjectPlatform: return IM_COL32(50, 180, 90, 220);
    case sro::nav::PassabilityKind::Oob: return IM_COL32(55, 55, 55, 255);
    }
    return IM_COL32(80, 80, 80, 255);
}
} // namespace

const char* TileFlagLabel(uint16_t flags) {
    if (IsTileOob(flags)) return "OOB (no terrain)";
    if (IsTileBlocked(flags)) return "Terrain blocked (bit 0)";
    return "Walkable terrain (objects may block via BMS)";
}

ImU32 TileMapColor(const sro::formats::NavTile& tile, int colorMode,
                   const std::vector<uint8_t>* objectFootprint,
                   const std::vector<sro::nav::PassabilityKind>* passability,
                   int tileIdx) {
    if (colorMode == kModeCombinedPassability && passability && tileIdx >= 0
        && tileIdx < (int)passability->size()) {
        return PassabilityKindColor((*passability)[static_cast<size_t>(tileIdx)]);
    }
    if (colorMode == kModeObjectCoverage) {
        if (IsTileOob(tile.Flags)) return IM_COL32(55, 55, 55, 255);
        if (objectFootprint && tileIdx >= 0 && tileIdx < (int)objectFootprint->size()
            && (*objectFootprint)[static_cast<size_t>(tileIdx)])
            return IM_COL32(50, 110, 220, 220);
        if (IsTileBlocked(tile.Flags)) return IM_COL32(210, 45, 45, 255);
        return IM_COL32(35, 45, 35, 180);
    }
    if (colorMode == kModeObjectBlockedFp) {
        if (IsTileOob(tile.Flags)) return IM_COL32(55, 55, 55, 255);
        if (objectFootprint && tileIdx >= 0 && tileIdx < (int)objectFootprint->size()
            && (*objectFootprint)[static_cast<size_t>(tileIdx)])
            return IM_COL32(255, 140, 30, 230);
        if (IsTileBlocked(tile.Flags)) return IM_COL32(210, 45, 45, 255);
        return IM_COL32(40, 90, 50, 200);
    }
    if (colorMode == kModeTextureId) {
        if (IsTileOob(tile.Flags)) return IM_COL32(40, 40, 40, 255);
        if (IsTileBlocked(tile.Flags)) return IM_COL32(150, 30, 30, 255);
        uint32_t h = (uint32_t)tile.TextureID * 2654435761u;
        return IM_COL32(40 + ((h >> 0) & 0x7F), 60 + ((h >> 8) & 0x7F), 180 + ((h >> 16) & 0x3F), 255);
    }
    if (colorMode == kModeCellId) {
        uint32_t h = (uint32_t)tile.CellID * 2654435761u;
        uint8_t r = 60 + ((h >> 0) & 0x7F);
        uint8_t g = 60 + ((h >> 8) & 0x7F);
        uint8_t b = 60 + ((h >> 16) & 0x7F);
        if (IsTileOob(tile.Flags)) return IM_COL32(40, 40, 40, 255);
        if (IsTileBlocked(tile.Flags)) return IM_COL32(200, 40, 40, 255);
        return IM_COL32(r, g, b, 255);
    }
    if (IsTileOob(tile.Flags)) return IM_COL32(55, 55, 55, 255);
    if (IsTileBlocked(tile.Flags)) return IM_COL32(210, 45, 45, 255);
    return IM_COL32(28, 32, 28, 200);
}

static void RebuildNavGpu(EditorContext& ctx, int rx, int ry) {
    if (!ctx.sroRegionManager || !ctx.sroRenderManager) return;
    auto* nav = ctx.sroRegionManager->GetNavMesh(rx, ry);
    if (nav && ctx.sroRenderManager->GetNvmRenderer())
        ctx.sroRenderManager->GetNvmRenderer()->RebuildNavmeshBuffers(nav, rx, ry);
}

static void PaintTileAt(EditorContext& ctx, sro::formats::NavMesh& nav, int rx, int ry,
                        int tx, int tz, bool setBlocked, std::vector<NavmeshFlagDelta>& deltas) {
    if (tx < 0 || tz < 0 || tx >= 96 || tz >= 96) return;
    const int idx = tz * 96 + tx;
    if (idx >= (int)nav.TileMap.size()) return;
    auto& tile = nav.TileMap[idx];
    if (tile.Flags == 0xFFFF) return;
    const uint16_t oldFlags = tile.Flags;
    if (setBlocked) tile.Flags |= 0x0001;
    else tile.Flags &= 0xFFFE;
    if (tile.Flags != oldFlags)
        deltas.push_back({idx, oldFlags, tile.Flags});
}

static void DrawPassabilityLegend(int colorMode) {
    if (colorMode == kModeCombinedPassability) {
        ImGui::TextDisabled("Green=terrain corridor  Teal=walkable platform  Orange=building BMS  Red=terrain blocked  Gray=OOB");
    } else if (colorMode == kModeObjectCoverage) {
        ImGui::TextDisabled("Blue=any object BMS coverage  Red=terrain blocked  Dark=open terrain corridor");
    } else if (colorMode == kModeObjectBlockedFp) {
        ImGui::TextDisabled("Orange=building BMS volume  Red=terrain blocked  Green=terrain corridor");
    }
}

TileMapCanvasResult DrawTileMapCanvas(EditorContext& ctx, sro::formats::NavMesh& nav, bool allowPaint) {
    TileMapCanvasResult result;
    auto& st = ctx.navmeshEditor;

    if (st.tileZoom < 2.0f) st.tileZoom = 2.0f;
    if (st.tileZoom > 12.0f) st.tileZoom = 12.0f;

    ImGui::SliderFloat("Zoom", &st.tileZoom, 2.0f, 12.0f, "%.0f px/tile");
    ImGui::SameLine();
    const char* colorModes[] = {
        "Walkable/Blocked/OOB",
        "CellQuad ID",
        "Texture ID",
        "Building BMS footprint",
        "Combined passability",
        "Object nav coverage"};
    ImGui::SetNextItemWidth(240);
    ImGui::Combo("Color By", &st.tileColorMode, colorModes, IM_ARRAYSIZE(colorModes));
    DrawPassabilityLegend(st.tileColorMode);

    std::vector<uint8_t> objectFootprint;
    std::vector<sro::nav::PassabilityKind> passability;
    int rx = 0, ry = 0;
    if (ctx.sroRegionManager) {
        rx = ctx.sroRegionManager->GetCurrentRx();
        ry = ctx.sroRegionManager->GetCurrentRy();
    }
    if (ctx.sroPlacements && ctx.sroRegionManager
        && (st.tileColorMode == kModeObjectBlockedFp
            || st.tileColorMode == kModeCombinedPassability
            || st.tileColorMode == kModeObjectCoverage)) {
        EnsureRegionBmsLoaded(ctx, rx, ry);
        if (st.tileColorMode == kModeObjectBlockedFp)
            sro::nav::BuildObjectBuildingFootprint(*ctx.sroPlacements, rx, ry, objectFootprint);
        else if (st.tileColorMode == kModeObjectCoverage)
            sro::nav::BuildObjectNavCoverageFootprint(*ctx.sroPlacements, rx, ry, objectFootprint);
        else
            sro::nav::BuildCombinedPassabilityMask(nav, *ctx.sroPlacements, rx, ry, passability);
    }

    const float tileSize = st.tileZoom;
    const float gridW = 96.0f * tileSize;
    const float gridH = 96.0f * tileSize;

    ImGui::BeginChild("TileMapScroll", ImVec2(-1, gridH + 8.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    const std::vector<uint8_t>* footprintPtr = objectFootprint.empty() ? nullptr : &objectFootprint;
    const std::vector<sro::nav::PassabilityKind>* passPtr = passability.empty() ? nullptr : &passability;

    for (int tz = 0; tz < 96; ++tz) {
        for (int tx = 0; tx < 96; ++tx) {
            const int idx = tz * 96 + tx;
            const auto& tile = nav.TileMap[idx];
            ImU32 fill = TileMapColor(tile, st.tileColorMode, footprintPtr, passPtr, idx);
            const ImVec2 p0(origin.x + tx * tileSize, origin.y + tz * tileSize);
            const ImVec2 p1(p0.x + tileSize, p0.y + tileSize);
            dl->AddRectFilled(p0, p1, fill);
            if (tileSize >= 3.0f)
                dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 40));
            if (idx == st.selectedTileIdx)
                dl->AddRect(p0, ImVec2(p1.x + 1, p1.y + 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }
    }

    ImGui::InvisibleButton("##tilemap_canvas", ImVec2(gridW, gridH));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    if (hovered || active) {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        result.hoverTx = (int)((mp.x - origin.x) / tileSize);
        result.hoverTz = (int)((mp.y - origin.y) / tileSize);
        if (result.hoverTx >= 0 && result.hoverTx < 96 && result.hoverTz >= 0 && result.hoverTz < 96) {
            const int idx = result.hoverTz * 96 + result.hoverTx;
            const auto& tile = nav.TileMap[idx];
            if (st.tileColorMode == kModeCombinedPassability && passPtr && idx < (int)passPtr->size()) {
                const auto kind = (*passPtr)[static_cast<size_t>(idx)];
                sro::nav::TileBmsStats bmsStats;
                if (ctx.sroPlacements)
                    sro::nav::QueryTileBmsStats(*ctx.sroPlacements, rx, ry, result.hoverTx, result.hoverTz, bmsStats);
                ImGui::SetTooltip(
                    "Tile (%d,%d) idx=%d\n%s\nTerrain: %s (0x%04X)\nObject: %d placements (%d building cells, %d platform cells)",
                    result.hoverTx, result.hoverTz, idx,
                    sro::nav::PassabilityVerdict(kind),
                    TileFlagLabel(tile.Flags), tile.Flags,
                    bmsStats.placementsOnTile,
                    bmsStats.buildingCells, bmsStats.platformCells);
            } else {
                sro::nav::TileNavDiagnostics diag;
                if (ctx.sroPlacements)
                    sro::nav::QueryTileNavDiagnostics(nav, *ctx.sroPlacements, rx, ry,
                                                      result.hoverTx, result.hoverTz, diag);
                float sampleH = 0.0f;
                if (!nav.HeightMap.empty())
                    sampleH = sro::nav::SampleHeightAtTileCenter(nav.HeightMap, result.hoverTx, result.hoverTz);
                const char* planeLabel = "none";
                if (diag.planeIndex >= 0 && diag.planeIndex < static_cast<int>(nav.PlaneTypeMap.size())) {
                    switch (nav.PlaneTypeMap[static_cast<size_t>(diag.planeIndex)]) {
                    case 1: planeLabel = "water"; break;
                    case 2: planeLabel = "ice"; break;
                    default: planeLabel = "none"; break;
                    }
                }
                const bool objBlocked = footprintPtr && idx < (int)footprintPtr->size()
                    && (*footprintPtr)[static_cast<size_t>(idx)];
                ImGui::SetTooltip(
                    "Tile (%d,%d) idx=%d\nCellID=%d  Flags=0x%04X  TextureID=%d\nHeight=%.2f  Plane[%d]=%s\n%s\n"
                    "QuadCell objects: %d  Internal edges: %d (%d blocked)",
                    result.hoverTx, result.hoverTz, idx, tile.CellID, tile.Flags, tile.TextureID,
                    sampleH, diag.planeIndex, planeLabel,
                    TileFlagLabel(tile.Flags),
                    (int)diag.quadCellObjIndices.size(), diag.crossingInternalEdges, diag.crossingBlockedEdges);
                (void)objBlocked;
            }
        } else {
            result.hoverTx = result.hoverTz = -1;
        }
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (result.hoverTx >= 0 && result.hoverTz >= 0) {
            st.selectedTileIdx = result.hoverTz * 96 + result.hoverTx;
            result.selectionChanged = true;
        }
    }

    const bool readOnly = IsReadOnlyColorMode(st.tileColorMode);
    if (allowPaint && !readOnly && active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        if (result.hoverTx >= 0 && result.hoverTz >= 0 && ctx.sroRegionManager) {
            const bool setBlocked = (st.paintMode == 0);
            std::vector<NavmeshFlagDelta> deltas;
            const int brush = std::max(1, (int)std::ceil(st.brushSize));
            for (int dz = -(brush - 1); dz <= brush - 1; ++dz) {
                for (int dx = -(brush - 1); dx <= brush - 1; ++dx)
                    PaintTileAt(ctx, nav, rx, ry, result.hoverTx + dx, result.hoverTz + dz, setBlocked, deltas);
            }
            if (!deltas.empty()) {
                ctx.sroRegionManager->MarkNavmeshDirty(rx, ry);
                ctx.MarkModified();
                RebuildNavGpu(ctx, rx, ry);
                result.tilesModified = true;
            }
        }
    }

    if (allowPaint && !readOnly && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (result.hoverTx >= 0 && result.hoverTz >= 0 && ctx.sroRegionManager) {
            const int idx = result.hoverTz * 96 + result.hoverTx;
            auto& tile = nav.TileMap[idx];
            if (tile.Flags != 0xFFFF) {
                const uint16_t old = tile.Flags;
                if (IsTileBlocked(tile.Flags)) tile.Flags &= 0xFFFE;
                else tile.Flags |= 0x0001;
                if (tile.Flags != old) {
                    ctx.sroRegionManager->MarkNavmeshDirty(rx, ry);
                    ctx.MarkModified();
                    RebuildNavGpu(ctx, rx, ry);
                    st.selectedTileIdx = idx;
                    result.tilesModified = true;
                    result.selectionChanged = true;
                }
            }
        }
    }

    ImGui::Dummy(ImVec2(gridW, 0));
    ImGui::EndChild();

    if (readOnly)
        ImGui::TextDisabled("Read-only analysis view — switch color mode to paint terrain tiles");
    else
        ImGui::TextDisabled("LMB drag=paint  RMB=toggle blocked  Yellow=selected");
    return result;
}

void FocusTileMapOnPlacement(EditorContext& ctx, const PlacementVM& p, int rx, int ry) {
    int minTx = 0, minTz = 0, maxTx = 0, maxTz = 0;
    if (!sro::nav::ComputePlacementFootprintBounds(p, rx, ry, minTx, minTz, maxTx, maxTz))
        return;
    const int cx = (minTx + maxTx) / 2;
    const int cz = (minTz + maxTz) / 2;
    ctx.navmeshEditor.selectedTileIdx = cz * 96 + cx;
    const int span = std::max(maxTx - minTx + 1, maxTz - minTz + 1);
    ctx.navmeshEditor.tileZoom = std::clamp(96.0f / static_cast<float>(span) * 0.85f, 4.0f, 12.0f);
}

} // namespace Ui
