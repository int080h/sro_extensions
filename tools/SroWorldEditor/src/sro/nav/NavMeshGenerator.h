#pragma once
#include "formats/NavMeshFormat.h"

namespace sro::nav {

class NavMeshGenerator {
public:
  // Rebuild CellQuads and InternalEdges from TileMap walkability grid.
  // Simplified greedy quad merge; may differ slightly from game client.
  static void RegenerateCells(formats::NavMesh& nav);
};

} // namespace sro::nav
