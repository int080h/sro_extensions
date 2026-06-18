#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace sro::formats {

// JMXVNVM 1000 - RTNavMeshTerrain (per non-dungeon region navmesh).
// Field layout mirrors the on-disk format exactly (see SilkroadDoc JMXVNVM).

// LinkEdge: links a GlobalEdge of one object instance to a GlobalEdge of another
// object instance available within the same RTNavMeshTerrain. Forms the inter-object
// AI navigation graph. Values of -1 are temporarily invalid (pending neighbor stream-in).
struct NavObjectEdge {
    int16_t LinkedObjID = -1;       // index into ObjectList
    int16_t LinkedObjEdgeID = -1;   // other object instance's edge
    int16_t EdgeID = -1;            // this object instance's edge
};

// MapObject: an instance of an RTNavMeshObj placed in the world.
// AssetID references an RTNavMeshObj through MapObjectIndex (object.ifo -> .bsr -> .cpd -> .bms).
// WorldUID = (RegionID << 16) | LocalUID, formed at runtime for global identity / dedupe.
struct NavObject {
    int32_t AssetID = 0;            // -> object.ifo entry -> .bsr/.cpd asset
    float PosX = 0, PosY = 0, PosZ = 0;  // LocalPosition (relative to region)
    int16_t Type = -1;              // -1 = Static, 0 = SkinnedNavMesh
    float Yaw = 0;                  // rotation around Y (height)
    int16_t LocalUID = 0;           // unique within region
    int16_t Short0 = 0;             // unknown
    uint8_t IsBig = 0;              // bypass distance culling (landmarks: gates, bridges)
    uint8_t IsStruct = 0;           // structure -> requires objectstring.ifo entry
    uint16_t RegionID = 0;          // origin RegionID (RID); owner when == file region
    std::vector<NavObjectEdge> Edges;  // LinkEdges to other instances

    uint32_t WorldUID() const { return (uint32_t(RegionID) << 16) | uint16_t(LocalUID); }
};

// RTNavMeshCellQuad: rectangular walkable cell (the actual pathfinding node).
// Generated from the TileMap as the biggest possible quad.
// Min size 20x20, max 960x960 (2x2 CellQuads per region).
struct NavCell {
    float MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;  // NavRect (local 2D)
    std::vector<uint16_t> ObjIndices;  // ObjectList indices inside this cell
};

// RTNavMeshEdgeGlobal (GlobalLinker): line + bidirectional link between border
// CellQuads of NEIGHBORING RTNavMeshTerrains. Entire side or sub-segment.
// AssocDirection/AssocCell/AssocRegion use -1 to mean Blocked.
struct NavGlobalEdge {
    float MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;  // NavLine (local 2D)
    uint8_t Flags = 0;             // EdgeFlag
    int8_t D0 = -1, D1 = -1;       // AssocDirection[0],[1] -> index into cell.m_EdgeList
    int16_t C0 = -1, C1 = -1;      // AssocCell[0],[1] -> index into pCell.m_CellList
    int16_t R0 = -1, R1 = -1;      // AssocRegion[0],[1] -> RegionID
};

// RTNavMeshEdgeInternal (LocalLinker): line + bidirectional link between CellQuads
// WITHIN the same RTNavMeshTerrain. Entire side or sub-segment.
struct NavInternalEdge {
    float MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;  // NavLine (local 2D)
    uint8_t Flags = 0;             // EdgeFlag
    int8_t D0 = -1, D1 = -1;       // AssocDirection[0],[1] -> index into cell.m_EdgeList
    int16_t C0 = -1, C1 = -1;      // AssocCell[0],[1] -> index into pCell.m_CellList
};

// NavMeshTile: 20x20 unit tile. The raw blocking grid used to GENERATE CellQuads.
// Flag: 1 = Blocked (bit 0). 0xFFFF = out of bounds. Split into 2 bytes (see wiki).
// TextureID is replaced at runtime with the SoundFlag for foot-step sounds.
struct NavTile {
    int32_t CellID = 0;            // the QuadCell this tile belongs to
    uint16_t Flags = 0;
    uint16_t TextureID = 0;        // -> TextureIndex (Tile2D.ifo)
};

struct NavMesh {
    std::string Signature = "JMXVNVM 1000";
    std::vector<NavObject> Objects;            // ObjectList
    uint32_t TotalCellCount = 0;
    uint32_t OpenCellCount = 0;                // walkable cell count
    std::vector<NavCell> Cells;                // QuadCellList
    uint32_t GlobalEdgeCount = 0;
    std::vector<NavGlobalEdge> GlobalEdges;    // GlobalEdgeList
    uint32_t InternalEdgeCount = 0;
    std::vector<NavInternalEdge> InternalEdges;// InternalEdgeList
    std::vector<NavTile> TileMap;              // 96x96
    std::vector<float> HeightMap;              // 97x97 vertex Y
    std::vector<uint8_t> PlaneTypeMap;         // 6x6: 0=None,1=Water,2=Ice
    std::vector<float> PlaneHeightMap;         // 6x6 plane heights (320x320 planes)

    // Round-trip format flags (detected on Read, preserved on Write). The wiki
    // documents only the 8-byte-tile + planes layout, but real data has 3 variants.
    bool TilesAre4Byte = false;  // true = tiles store CellID only (no Flags/TextureID)
    bool HasPlanes = true;       // true = PlaneTypeMap + PlaneHeightMap present

    static constexpr int kTileExtent = 96;     // tiles per axis
    static constexpr int kHeightExtent = 97;   // height samples per axis
    static constexpr int kPlaneExtent = 6;     // planes per axis
    static constexpr float kTileSize = 20.0f;  // units per tile
    static constexpr float kPlaneSize = 320.0f;// units per plane
};

} // namespace sro::formats
