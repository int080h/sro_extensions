#pragma once
#include <cstdint>

struct HeightDelta {
    int BlockIdx;
    int VertexIdx;
    float OldHeight;
    float NewHeight;
};

struct TextureDelta {
    int BlockIdx;
    int TileIdx;
    uint16_t OldTileVal;
    uint16_t NewTileVal;
    uint16_t OldTextureData[4];
    uint16_t NewTextureData[4];
};

struct NavmeshFlagDelta {
    int TileIdx;
    uint16_t OldFlags;
    uint16_t NewFlags;
};
