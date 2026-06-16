#pragma once
#include "imgui.h"
#include <cmath>

namespace MapPlayerIcon {

// Rotated map marker quad (mm_sign_character.ddj default points east / +X on map).
inline void DrawRotatedMapIcon(ImDrawList* drawList, ImTextureID texture, ImVec2 center, float size,
                               float angleRad, ImU32 tint = IM_COL32(255, 255, 255, 255)) {
    if (!drawList || !texture || size <= 0.0f) return;

    const float half = size * 0.5f;
    const float cosA = std::cos(angleRad);
    const float sinA = std::sin(angleRad);
    const ImVec2 local[4] = {
        ImVec2(-half, -half),
        ImVec2(half, -half),
        ImVec2(half, half),
        ImVec2(-half, half),
    };
    ImVec2 quad[4];
    for (int i = 0; i < 4; ++i) {
        quad[i].x = center.x + local[i].x * cosA - local[i].y * sinA;
        quad[i].y = center.y + local[i].x * sinA + local[i].y * cosA;
    }
    drawList->AddImageQuad(texture, quad[0], quad[1], quad[2], quad[3],
                           ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f), tint);
}

} // namespace MapPlayerIcon
