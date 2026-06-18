#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "core/Math.h"

struct BmsVertex {
    float X, Y, Z;     // Position
    float NX, NY, NZ;  // Normal
    float U, V;        // UV coordinates
};

struct BmsFace {
    uint16_t A, B, C;  // Indices
};

struct BmsVertexSkin {
    uint8_t boneIdx1 = 0;
    uint8_t boneIdx2 = 0;
    float weight1 = 0.f;
    float weight2 = 0.f;
};

// RTNavMeshObj -- the per-object navmesh embedded in a .bms NavMeshOffset section.
// This is the real object collision/navigation geometry the game uses (gates,
// bridges, buildings). Referenced by JMXVNVM ObjectList via AssetID.
struct BmsNavVertex {
    Vector3 Position{};
    uint8_t BisectorIndex = 0;   // index into bisector cache
};

// RTNavMeshCellTri -- triangular nav cell (ObjectGround).
struct BmsNavCellTri {
    uint16_t V0 = 0, V1 = 0, V2 = 0;  // indices into NavVertices
    uint16_t Flag = 0;
    uint8_t EventZoneData = 0;        // only present when NavFlag & 2
};

// PrimMeshNavEdge -- outline (border) or inline (internal) nav edge.
struct BmsNavEdge {
    uint16_t SrcVertex = 0, DstVertex = 0;  // indices into NavVertices
    uint16_t SrcCell = 0, DstCell = 0xFFFF; // indices into NavCells; DstCell 0xFFFF = none
    uint8_t Flag = 0;                       // PrimMeshNavEdgeFlag
    uint8_t EventZoneData = 0;              // only present when NavFlag & 1
};

struct BmsNavEvent {
    std::string Name;   // NavCallBack2 related
};

// OutlineLookupGrid -- acceleration structure for outline-edge queries.
struct BmsNavOutlineGrid {
    Vector2 Origin{};
    uint32_t Width = 0, Height = 0, CellCount = 0;
    std::vector<std::vector<uint16_t>> OutlineIndices;  // per grid cell
};

struct BmsNavMesh {
    bool HasData = false;
    uint32_t NavFlag = 0;   // 0=None,1=Edge,2=Cell,4=Event (controls optional fields)
    std::vector<BmsNavVertex> Vertices;
    std::vector<BmsNavCellTri> Cells;        // RTNavMeshCellTri
    std::vector<BmsNavEdge> OutlineEdges;    // NavOutlineEdges (border)
    std::vector<BmsNavEdge> InlineEdges;     // NavInlineEdges (internal)
    std::vector<BmsNavEvent> Events;
    BmsNavOutlineGrid OutlineGrid;
};

class BmsMesh {
public:
    std::string Signature;
    std::vector<BmsVertex> Vertices;
    std::vector<BmsFace> Faces;
    std::string Material;
    std::vector<std::string> SkinBones;
    std::vector<BmsVertexSkin> Skin;
    BmsNavMesh NavMesh;   // per-object navmesh (RTNavMeshObj)

    bool Read(const std::wstring& path);
    std::vector<float> GetFlattenedVertices() const;
    std::vector<uint32_t> GetFlattenedIndices() const;
};

// Read full BMS file, replace NavMeshOffset section, write back. Returns false if navMeshOffset is 0.
bool WriteNavMeshOffset(const std::wstring& path, const BmsNavMesh& nav);
