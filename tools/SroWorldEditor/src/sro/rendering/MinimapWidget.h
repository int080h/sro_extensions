#pragma once
#include <d3d9.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "Rendering/Texture.h"
#include "Core/Math.h"
#include "sro/WorldMapCoords.h"
#include "sro_map/MapTextureCache.h"

class MinimapTileIndex;

// In-game-style corner minimap HUD (CMinimapWidget_LoadRegionTiles @ 0x577030).
class MinimapWidget {
public:
    using PathLookupFn = std::function<std::optional<std::wstring>(int rx, int ry)>;

    int CenterRx = 97;
    int CenterRy = 167;
    float PlayerLocalX = 960.0f;
    float PlayerLocalZ = 960.0f;
    float PlayerHeadingRad = 0.0f;
    bool HasPlayer = false;

    MinimapWidget(LPDIRECT3DDEVICE9 device, MapTextureCache* mapCache);
    void SetPathLookup(PathLookupFn lookup);
    void SetOverlayTextureManager(TextureManager* texMgr, const std::wstring& clientPath);
    void SetCenterRegion(int rx, int ry);
    void SetPlayerPosition(float localX, float localZ, float headingRad = 0.0f);
    void InvalidateCachedTextures();
    void NotifyIndexUpdated();

    bool HasTiles() const { return !m_tiles.empty(); }
    bool AreAllTilesReady() const;
    bool NeedsRedraw() const;
    void MarkRendered(int tilesDrawn);

    int RenderImGuiTiles(float screenX, float screenY, float size);
    void RenderImGuiOverlay(float screenX, float screenY, float size, bool indexComplete,
                            float indexProgress);

private:
    struct TileSlot {
        int Rx = 0;
        int Ry = 0;
        std::wstring Path;
    };

    LPDIRECT3DDEVICE9 m_device = nullptr;
    MapTextureCache* m_mapCache = nullptr;
    TextureManager* m_overlayTexMgr = nullptr;
    std::wstring m_clientPath;
    PathLookupFn m_pathLookup;
    std::vector<TileSlot> m_tiles;
    Texture* m_playerDotTex = nullptr;
    int m_lastRenderedCenterRx = -1;
    int m_lastRenderedCenterRy = -1;

    void RefreshTiles();
};
