#pragma once
#include "formats/MapFormats.h"
#include "core/Math.h"
#include "parsers/BmsParser.h"
#include <array>
#include <cstdint>
#include <string>

// Real game object-collision model (no app-invented AABB/flags).
// A placement's collision/nav geometry comes from the resource chain:
//   ObjID -> object.ifo -> .bsr (CollisionPath + collisionBox0/1) -> .bms NavMeshOffset (RTNavMeshObj)
//   or -> .cpd (collisionResourcePath) -> .bms NavMeshOffset
// The BMS NavMeshOffset holds the RTNavMeshCellTri cells + NavOutline/InlineEdges the
// game uses for AI pathfinding on/around the object. The BSR collisionBox0/1 are the
// authored collision boxes. None of this is editor-editable; it is read straight from disk.
struct ObjectCollision {
    bool HasAuthoredBox = false;          // BSR provided collisionBox0/1
    std::array<float, 24> Box0{};
    std::array<float, 24> Box1{};
    uint32_t RequireCollisionMatrix = 0;
    std::array<uint8_t, 64> CollisionMatrix{};

    std::string CollisionMeshPath;        // resolved .bms carrying the NavMeshOffset
    const BmsNavMesh* NavMesh = nullptr;  // per-object RTNavMeshObj (non-owning, cache-backed)
};

struct PlacementVM {
    sro::formats::MapObject Object;
    std::string BsrPath;
    int BlockZ = 0, BlockX = 0, Lod = 0, Index = 0;
    int LoadedRx = 0, LoadedRy = 0;
    ObjectCollision Collision;
};
