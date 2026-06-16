#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace sro::formats {

struct NavObjectEdge {
    int16_t LinkedObjID = 0;
    int16_t LinkedObjEdgeID = 0;
    int16_t EdgeID = 0;
};

struct NavObject {
    int32_t ResID = 0;
    float PosX = 0, PosY = 0, PosZ = 0;
    int16_t IsStatic = 0;
    float Yaw = 0;
    int16_t UID = 0;
    int16_t UnkShort = 0;
    uint8_t IsBig = 0;
    uint8_t IsStruct = 0;
    uint16_t RegionID = 0;
    std::vector<NavObjectEdge> Edges;
};

struct NavCell {
    float MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
    std::vector<uint16_t> ObjIndices;
};

struct NavGlobalEdge {
    float MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
    uint8_t Flags = 0;
    int8_t D0 = 0, D1 = 0;
    int16_t C0 = 0, C1 = 0, R0 = 0, R1 = 0;
};

struct NavInternalEdge {
    float MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
    uint8_t Flags = 0;
    int8_t D0 = 0, D1 = 0;
    int16_t C0 = 0, C1 = 0;
};

struct NavTile {
    int32_t CellID = 0;
    uint16_t Flags = 0;
    uint16_t TextureID = 0;
};

struct NavMesh {
    std::string Signature = "JMXVNVM 1000";
    std::vector<NavObject> Objects;
    uint32_t TotalCellCount = 0;
    uint32_t OpenCellCount = 0;
    std::vector<NavCell> Cells;
    uint32_t GlobalEdgeCount = 0;
    std::vector<NavGlobalEdge> GlobalEdges;
    uint32_t InternalEdgeCount = 0;
    std::vector<NavInternalEdge> InternalEdges;
    std::vector<NavTile> TileMap;
    std::vector<float> HeightMap;
    std::vector<uint8_t> PlaneTypeMap;
    std::vector<float> PlaneHeightMap;
};

} // namespace sro::formats
