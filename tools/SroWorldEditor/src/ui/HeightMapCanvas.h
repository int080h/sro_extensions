#pragma once
#include "formats/NavMeshFormat.h"
#include "imgui.h"

namespace Ui {

// 97x97 height heatmap; click picks vertex index (syncs with Terrain Nav height editor).
void DrawHeightMapCanvas(sro::formats::NavMesh& nav, int* editVertexIdx);

} // namespace Ui
