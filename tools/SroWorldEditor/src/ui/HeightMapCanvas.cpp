#include "ui/HeightMapCanvas.h"
#include "sro/nav/TileCoord.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Ui {

namespace {
constexpr int kSamples = sro::nav::kHeightSamples;

ImU32 HeightToColor(float h, float minH, float maxH) {
    if (maxH <= minH)
        return IM_COL32(100, 100, 100, 255);
    const float t = std::clamp((h - minH) / (maxH - minH), 0.0f, 1.0f);
    const uint8_t r = static_cast<uint8_t>(40.0f + t * 80.0f);
    const uint8_t g = static_cast<uint8_t>(60.0f + t * 140.0f);
    const uint8_t b = static_cast<uint8_t>(180.0f - t * 120.0f);
    return IM_COL32(r, g, b, 255);
}
} // namespace

void DrawHeightMapCanvas(sro::formats::NavMesh& nav, int* editVertexIdx) {
    if (nav.HeightMap.size() != static_cast<size_t>(kSamples * kSamples)) {
        ImGui::TextDisabled("HeightMap size mismatch (expected 97x97).");
        return;
    }

    float minH = nav.HeightMap[0], maxH = nav.HeightMap[0];
    for (float h : nav.HeightMap) {
        minH = std::min(minH, h);
        maxH = std::max(maxH, h);
    }

    const float cell = 4.0f;
    const float gridW = kSamples * cell;
    const float gridH = kSamples * cell;

    ImGui::BeginChild("HeightMapHeat", ImVec2(-1, gridH + 8.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    for (int vz = 0; vz < kSamples; ++vz) {
        for (int vx = 0; vx < kSamples; ++vx) {
            const int idx = sro::nav::HeightVertexIndex(vx, vz);
            const float h = nav.HeightMap[static_cast<size_t>(idx)];
            const ImVec2 p0(origin.x + vx * cell, origin.y + vz * cell);
            const ImVec2 p1(p0.x + cell, p0.y + cell);
            dl->AddRectFilled(p0, p1, HeightToColor(h, minH, maxH));
            if (editVertexIdx && *editVertexIdx == idx)
                dl->AddRect(p0, p1, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }
    }

    ImGui::InvisibleButton("##heightmap_canvas", ImVec2(gridW, gridH));
    if (ImGui::IsItemHovered()) {
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const int vx = static_cast<int>((mp.x - origin.x) / cell);
        const int vz = static_cast<int>((mp.y - origin.y) / cell);
        if (vx >= 0 && vz >= 0 && vx < kSamples && vz < kSamples) {
            const int idx = sro::nav::HeightVertexIndex(vx, vz);
            const float h = nav.HeightMap[static_cast<size_t>(idx)];
            ImGui::SetTooltip("Vertex (%d,%d) idx=%d\nHeight=%.2f", vx, vz, idx, h);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && editVertexIdx)
                *editVertexIdx = idx;
        }
    }

    ImGui::Dummy(ImVec2(gridW, 0));
    ImGui::EndChild();
    ImGui::TextDisabled("LMB pick vertex  |  Blue=low  Brown=high  (%.2f .. %.2f)", minH, maxH);
}

} // namespace Ui
