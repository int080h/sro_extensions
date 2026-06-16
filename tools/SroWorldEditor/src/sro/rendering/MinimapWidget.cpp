#include "MinimapWidget.h"
#include "MapPlayerIcon.h"
#include "sro_map/MinimapTileIndex.h"
#include "imgui.h"
#include "Core/Log.h"
#include <cmath>
#include <cstdio>

MinimapWidget::MinimapWidget(LPDIRECT3DDEVICE9 device, MapTextureCache* mapCache)
    : m_device(device), m_mapCache(mapCache) {}

void MinimapWidget::SetPathLookup(PathLookupFn lookup) {
    m_pathLookup = std::move(lookup);
    RefreshTiles();
}

void MinimapWidget::SetOverlayTextureManager(TextureManager* texMgr, const std::wstring& clientPath) {
    m_overlayTexMgr = texMgr;
    m_clientPath = clientPath;
    m_playerDotTex = nullptr;
}

void MinimapWidget::SetCenterRegion(int rx, int ry) {
    CenterRx = rx;
    CenterRy = ry;
    RefreshTiles();
}

void MinimapWidget::SetPlayerPosition(float localX, float localZ, float headingRad) {
    PlayerLocalX = localX;
    PlayerLocalZ = localZ;
    PlayerHeadingRad = headingRad;
    HasPlayer = true;
}

void MinimapWidget::InvalidateCachedTextures() { m_playerDotTex = nullptr; }

void MinimapWidget::NotifyIndexUpdated() { RefreshTiles(); }

bool MinimapWidget::AreAllTilesReady() const {
    if (!m_mapCache || m_tiles.empty()) return false;
    for (const auto& tile : m_tiles) {
        if (!m_mapCache->IsReady(tile.Path)) return false;
    }
    return true;
}

bool MinimapWidget::NeedsRedraw() const {
    if (CenterRx != m_lastRenderedCenterRx || CenterRy != m_lastRenderedCenterRy) return true;
    if (m_tiles.empty()) return true;
    return !AreAllTilesReady();
}

void MinimapWidget::MarkRendered(int tilesDrawn) {
    if (tilesDrawn <= 0) return;
    m_lastRenderedCenterRx = CenterRx;
    m_lastRenderedCenterRy = CenterRy;
}

void MinimapWidget::RefreshTiles() {
    m_tiles.clear();
    m_tiles.reserve(9);
    if (!m_pathLookup) return;

    for (int drx = -1; drx <= 1; ++drx) {
        for (int dry = -1; dry <= 1; ++dry) {
            int rx = 0;
            int ry = 0;
            sro::map::MinimapHudNeighborRegion(drx, dry, CenterRx, CenterRy, rx, ry);
            if (rx < 0 || rx > 255 || ry < 0 || ry > 255) continue;
            auto path = m_pathLookup(rx, ry);
            if (!path) continue;
            if (sro::map::PathHasSpacedCoords(*path)) {
                LogMsgW(L"[MinimapWidget] Rejecting spaced minimap path: " + *path);
                continue;
            }
            TileSlot slot;
            slot.Rx = rx;
            slot.Ry = ry;
            slot.Path = *path;
            if (m_mapCache) m_mapCache->Pin(slot.Path);
            m_tiles.push_back(std::move(slot));
        }
    }
}

int MinimapWidget::RenderImGuiTiles(float screenX, float screenY, float size) {
    if (!m_mapCache || m_tiles.empty() || size <= 0.0f) return 0;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const float tileSize = size / 3.0f;
    int tilesDrawn = 0;

    for (const auto& tile : m_tiles) {
        int col = 0;
        int row = 0;
        sro::map::MinimapHudCellToGrid(tile.Rx, tile.Ry, CenterRx, CenterRy, col, row);
        if (col < 0 || col > 2 || row < 0 || row > 2) continue;

        Texture* tex = m_mapCache->Acquire(tile.Path, MapLoadPolicy::NoLoad);
        if (!tex || !tex->pTexture) {
            tex = m_mapCache->Preload(tile.Path);
        }
        if (!tex || !tex->pTexture) continue;

        const ImVec2 p0(screenX + col * tileSize, screenY + row * tileSize);
        const ImVec2 p1(screenX + (col + 1) * tileSize, screenY + (row + 1) * tileSize);
        drawList->AddImage(reinterpret_cast<ImTextureID>(tex->pTexture), p0, p1);
        ++tilesDrawn;
    }

    return tilesDrawn;
}

void MinimapWidget::RenderImGuiOverlay(float screenX, float screenY, float size, bool indexComplete,
                                     float indexProgress) {
    if (size <= 0.0f) return;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 hudMin(screenX, screenY);
    const ImVec2 hudMax(screenX + size, screenY + size);

    const bool tilesReady = HasTiles() && AreAllTilesReady();
    if (!tilesReady) {
        drawList->AddRectFilled(hudMin, hudMax, IM_COL32(8, 12, 20, 200));
        drawList->AddRect(hudMin, hudMax, IM_COL32(235, 175, 40, 255), 0.0f, 0, 2.0f);

        const char* statusText = !indexComplete ? "Minimap..."
                                : !HasTiles() ? "No tiles"
                                              : "Loading...";
        ImVec2 textSize = ImGui::CalcTextSize(statusText);
        drawList->AddText(
            ImVec2(screenX + size * 0.5f - textSize.x * 0.5f, screenY + size * 0.5f - textSize.y * 0.5f),
            IM_COL32(200, 210, 230, 255), statusText);
        if (!indexComplete && indexProgress > 0.0f) {
            char pct[16];
            snprintf(pct, sizeof(pct), "%.0f%%", indexProgress * 100.0f);
            ImVec2 pctSize = ImGui::CalcTextSize(pct);
            drawList->AddText(
                ImVec2(screenX + size * 0.5f - pctSize.x * 0.5f, screenY + size * 0.5f + textSize.y * 0.5f),
                IM_COL32(160, 190, 255, 255), pct);
        }
        return;
    }

    drawList->AddRect(hudMin, hudMax, IM_COL32(235, 175, 40, 255), 0.0f, 0, 2.0f);

    if (!HasPlayer) return;
    if (m_overlayTexMgr && (!m_playerDotTex || !m_playerDotTex->pTexture)) {
        m_playerDotTex = m_overlayTexMgr->GetTexture(
            m_clientPath + L"/Media/interface/minimap/mm_sign_character.ddj", false);
    }
    const float tileSize = size / 3.0f;
    const float u = sro::map::MinimapHudU(PlayerLocalX);
    const float v = sro::map::MinimapHudVScreen(PlayerLocalZ);
    const float dotX = screenX + tileSize + u * tileSize;
    const float dotY = screenY + tileSize + v * tileSize;
    const float iconSize = (std::max)(12.0f, size * 0.16f);
    if (m_playerDotTex && m_playerDotTex->pTexture) {
        MapPlayerIcon::DrawRotatedMapIcon(drawList,
            reinterpret_cast<ImTextureID>(m_playerDotTex->pTexture), ImVec2(dotX, dotY), iconSize, PlayerHeadingRad);
    }
}
