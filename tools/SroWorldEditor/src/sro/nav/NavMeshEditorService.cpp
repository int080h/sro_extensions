#include "nav/NavMeshEditorService.h"
#include "nav/NavEdgeSemantics.h"
#include <queue>
#include <set>
#include <algorithm>

namespace sro::nav {

NavMeshEditorService::ValidationResult NavMeshEditorService::ValidateNavMesh(const formats::NavMesh& nav) const {
    ValidationResult result;

    auto addError = [&](const std::string& msg) {
        result.ok = false;
        result.issues.push_back({msg, true});
    };
    auto addWarning = [&](const std::string& msg) {
        result.issues.push_back({msg, false});
    };

    if (nav.TileMap.size() != 96u * 96u)
        addError("TileMap must be 96x96 (" + std::to_string(nav.TileMap.size()) + " entries)");
    if (nav.HeightMap.size() != 97u * 97u)
        addError("HeightMap must be 97x97 (" + std::to_string(nav.HeightMap.size()) + " entries)");

    if (nav.TilesAre4Byte)
        addWarning("TilesAre4Byte variant: Flags/TextureID not stored on disk");

    const uint32_t countedOpen = [&]() {
        uint32_t n = 0;
        for (const auto& t : nav.TileMap) {
            if (t.Flags == 0xFFFF) continue;
            if ((t.Flags & 0x0001) == 0) ++n;
        }
        return n;
    }();
    if (nav.OpenCellCount != countedOpen)
        addWarning("OpenCellCount (" + std::to_string(nav.OpenCellCount) + ") != walkable tiles (" +
                   std::to_string(countedOpen) + ")");

    const int cellCount = static_cast<int>(nav.Cells.size());
    for (const auto& edge : nav.InternalEdges) {
        if (edge.C0 < 0 || edge.C0 >= cellCount || edge.C1 < 0 || edge.C1 >= cellCount)
            addError("Internal edge references invalid cell index");
        if (ClassifyTerrainEdge(edge.Flags, edge.D0, edge.D1, edge.C0, edge.C1) ==
            TerrainEdgePassability::Disconnected)
            addWarning("Internal edge has disconnected AssocCell/Direction");
    }
    for (const auto& edge : nav.GlobalEdges) {
        if (edge.C0 < 0 || edge.C0 >= cellCount || edge.C1 < 0 || edge.C1 >= cellCount)
            addError("Global edge references invalid cell index");
    }

    int brokenObjLinks = 0;
    for (const auto& obj : nav.Objects) {
        for (const auto& edge : obj.Edges) {
            if (edge.LinkedObjID < 0 || edge.LinkedObjID >= static_cast<int>(nav.Objects.size()))
                ++brokenObjLinks;
        }
    }
    if (brokenObjLinks > 0)
        addWarning("Found " + std::to_string(brokenObjLinks) + " broken object edge links");

    std::set<int> connectedCells;
    for (const auto& edge : nav.InternalEdges) {
        if (edge.C0 >= 0 && edge.C0 < cellCount) connectedCells.insert(edge.C0);
        if (edge.C1 >= 0 && edge.C1 < cellCount) connectedCells.insert(edge.C1);
    }
    for (const auto& edge : nav.GlobalEdges) {
        if (edge.C0 >= 0 && edge.C0 < cellCount) connectedCells.insert(edge.C0);
        if (edge.C1 >= 0 && edge.C1 < cellCount) connectedCells.insert(edge.C1);
    }
    const int isolated = cellCount - static_cast<int>(connectedCells.size());
    if (cellCount > 0 && isolated > 0)
        addWarning(std::to_string(isolated) + " cell(s) have no edge connections");

    return result;
}

static bool EdgeAllowsTraversal(const formats::NavInternalEdge& edge, int fromCell, int toCell) {
    if (fromCell == edge.C0 && toCell == edge.C1) {
        if (edge.D0 < 0) return false;
        if ((edge.Flags & 2) != 0) return false;
        return ClassifyTerrainEdge(edge.Flags, edge.D0, edge.D1, edge.C0, edge.C1) !=
               TerrainEdgePassability::Blocked;
    }
    if (fromCell == edge.C1 && toCell == edge.C0) {
        if (edge.D1 < 0) return false;
        if ((edge.Flags & 1) != 0) return false;
        return ClassifyTerrainEdge(edge.Flags, edge.D0, edge.D1, edge.C0, edge.C1) !=
               TerrainEdgePassability::Blocked;
    }
    return false;
}

static bool GlobalEdgeAllowsTraversal(const formats::NavGlobalEdge& edge, int fromCell, int toCell) {
    if (fromCell == edge.C0 && toCell == edge.C1) {
        if (edge.D0 < 0) return false;
        if ((edge.Flags & 2) != 0) return false;
        return ClassifyTerrainEdge(edge.Flags, edge.D0, edge.D1, edge.C0, edge.C1) !=
               TerrainEdgePassability::Blocked;
    }
    if (fromCell == edge.C1 && toCell == edge.C0) {
        if (edge.D1 < 0) return false;
        if ((edge.Flags & 1) != 0) return false;
        return ClassifyTerrainEdge(edge.Flags, edge.D0, edge.D1, edge.C0, edge.C1) !=
               TerrainEdgePassability::Blocked;
    }
    return false;
}

std::vector<int> NavMeshEditorService::FindPathBFS(const formats::NavMesh& nav, int startCell, int endCell) {
    if (startCell < 0 || startCell >= static_cast<int>(nav.Cells.size())) return {};
    if (endCell < 0 || endCell >= static_cast<int>(nav.Cells.size())) return {};
    if (startCell == endCell) return {startCell};

    std::vector<std::vector<int>> adjacency(nav.Cells.size());

    for (const auto& edge : nav.InternalEdges) {
        if (edge.C0 >= 0 && edge.C0 < static_cast<int>(nav.Cells.size()) &&
            edge.C1 >= 0 && edge.C1 < static_cast<int>(nav.Cells.size())) {
            if (EdgeAllowsTraversal(edge, edge.C0, edge.C1))
                adjacency[edge.C0].push_back(edge.C1);
            if (EdgeAllowsTraversal(edge, edge.C1, edge.C0))
                adjacency[edge.C1].push_back(edge.C0);
        }
    }
    for (const auto& edge : nav.GlobalEdges) {
        if (edge.C0 >= 0 && edge.C0 < static_cast<int>(nav.Cells.size()) &&
            edge.C1 >= 0 && edge.C1 < static_cast<int>(nav.Cells.size())) {
            if (GlobalEdgeAllowsTraversal(edge, edge.C0, edge.C1))
                adjacency[edge.C0].push_back(edge.C1);
            if (GlobalEdgeAllowsTraversal(edge, edge.C1, edge.C0))
                adjacency[edge.C1].push_back(edge.C0);
        }
    }

    std::vector<int> parent(nav.Cells.size(), -1);
    std::vector<bool> visited(nav.Cells.size(), false);
    std::queue<int> q;
    q.push(startCell);
    visited[startCell] = true;

    while (!q.empty()) {
        int current = q.front();
        q.pop();
        if (current == endCell) break;
        for (int neighbor : adjacency[current]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                q.push(neighbor);
            }
        }
    }

    if (!visited[endCell]) return {};

    std::vector<int> path;
    for (int cur = endCell; cur != -1; cur = parent[cur])
        path.push_back(cur);
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace sro::nav
