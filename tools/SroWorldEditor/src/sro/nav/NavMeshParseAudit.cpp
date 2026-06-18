#include "sro/nav/NavMeshParseAudit.h"

namespace sro::nav {

std::vector<FieldConsumer> NavMeshFieldConsumers() {
    return {
        {"JMXVNVM", "ObjectList", "viz+NavMeshContext+validation"},
        {"JMXVNVM", "LinkEdges", "viz+passability+NavMeshContext"},
        {"JMXVNVM", "QuadCellList", "viz+passability+hover"},
        {"JMXVNVM", "OpenCellCount", "validation"},
        {"JMXVNVM", "GlobalEdgeList", "viz+passability+BFS"},
        {"JMXVNVM", "InternalEdgeList", "viz+passability+BFS"},
        {"JMXVNVM", "TileMap.CellID", "viz+hover+passability"},
        {"JMXVNVM", "TileMap.Flags", "viz+passability+paint"},
        {"JMXVNVM", "TileMap.TextureID", "viz+runtime-only-footstep"},
        {"JMXVNVM", "HeightMap", "viz+Y offset"},
        {"JMXVNVM", "PlaneTypeMap", "viz"},
        {"JMXVNVM", "PlaneHeightMap", "viz"},
        {"JMXVBMS", "NavVertices", "viz+transform"},
        {"JMXVBMS", "NavCells", "viz+passability"},
        {"JMXVBMS", "OutlineEdges", "viz+passability+classification"},
        {"JMXVBMS", "InlineEdges", "viz"},
        {"JMXVBMS", "Events", "diagnostics"},
        {"JMXVBMS", "OutlineLookupGrid", "diagnostics+hover"},
        {"JMXVBMS", "BisectorIndex", "runtime-only"},
    };
}

std::vector<std::string> AuditNavMeshUsage(const sro::formats::NavMesh& nav) {
    std::vector<std::string> warnings;
    if (nav.TileMap.size() != 96u * 96u)
        warnings.push_back("TileMap size != 96x96");
    if (nav.HeightMap.size() != 97u * 97u)
        warnings.push_back("HeightMap size != 97x97");
    if (nav.TilesAre4Byte)
        warnings.push_back("TilesAre4Byte: Flags/TextureID absent on disk");
    if (!nav.HasPlanes)
        warnings.push_back("HasPlanes=false: plane maps absent");
    if (nav.OpenCellCount != nav.TotalCellCount && nav.InternalEdges.empty())
        warnings.push_back("OpenCellCount mismatch but no internal edges");
    return warnings;
}

std::vector<std::string> AuditBmsNavUsage(const BmsNavMesh& bms, uint32_t navFlag) {
    std::vector<std::string> warnings;
    if (!bms.HasData)
        return warnings;
    if ((navFlag & 1) && bms.OutlineEdges.empty() && bms.InlineEdges.empty())
        warnings.push_back("NavFlag&Edge but no edges");
    if ((navFlag & 2) && bms.Cells.empty())
        warnings.push_back("NavFlag&Cell but no cells");
    if ((navFlag & 4) && bms.Events.empty())
        warnings.push_back("NavFlag&Event but no events");
    return warnings;
}

} // namespace sro::nav
