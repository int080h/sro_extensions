#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "core/Math.h"

namespace sro::formats {

// JMXVDOF - RTNavMeshDungeon (per-dungeon navmesh). Distinct from JMXVNVM terrain.
// A dungeon is split into blocks (rooms) with portal-based connectivity, a 3D voxel
// lookup grid (200x200x200 units/cell), off-mesh links, labels (rooms/floors) and groups.

struct DofFogParam {
    uint32_t Color = 0;
    float NearPlane = 0;
    float FarPlane = 0;
    float Intensity = 0;
    uint8_t HasHeightFog = 0;
    float HeightFog[4] = {0, 0, 0, 0};
};

// Block object (placed inside a dungeon block). Flag: 0=None, 2=ColObj, 4=WaterObj.
struct DofBlockObject {
    std::string Name;
    std::string Path;        // *.bsr
    Vector3 Position{};
    Vector3 Rotation{};
    Vector3 Scale{1, 1, 1};
    uint32_t Flag = 0;
    uint32_t Int0 = 0;
    float RadiusSqrt = 0;
    uint32_t WaterColor = 0;  // only valid when Flag & 4
};

struct DofLight {
    std::string Name;
    Vector3 Position{};
    float Color0[3] = {0, 0, 0};  // Diffuse?
    float Color1[3] = {0, 0, 0};  // Ambient?
    float Color2[3] = {0, 0, 0};  // Specular?
    float Float0 = 0, Float1 = 0, Float2 = 0;  // Attenuation?
};

struct DofBlock {
    std::string Path;
    std::string Name;
    uint32_t UnkUInt0 = 0;
    Vector3 Position{};
    float Yaw = 0;
    uint32_t IsEntrance = 0;
    float CollisionBox0[24] = {0};  // raw authored collision box
    uint32_t UnkUInt1 = 0;
    DofFogParam Fog;
    uint8_t UnkByte1 = 0;
    Vector3 UnkVector0{};
    Vector3 UnkVector1{};
    uint32_t UnkUInt2 = 0;
    std::string UnkString;
    uint32_t RoomIndex = 0;
    uint32_t FloorIndex = 0;
    std::vector<uint32_t> ConnectedBlockIndices;  // walkable neighbors
    std::vector<uint32_t> VisibleBlockIndices;    // portal-visible neighbors
    std::vector<DofBlockObject> Objects;
    std::vector<DofLight> Lights;
};

// Voxel of the 3D block lookup grid. 200x200x200 units. ID packs Y/Z/X indices.
struct DofVoxel {
    uint32_t ID = 0;
    std::vector<uint32_t> BlockIndices;
    uint32_t X() const { return (ID >> 0) & 0x7FF; }
    uint32_t Y() const { return (ID >> 21) & 0x7FF; }
    uint32_t Z() const { return (ID >> 11) & 0x7FF; }
};

struct DofLink {
    std::vector<uint32_t> BlockIndices;  // neighbor blocks (off-mesh link)
};

struct DofGroup {
    std::string Name;
    uint32_t Flag = 0;   // 0 or 1 -> Service?
    std::vector<uint32_t> BlockIndices;
};

struct DofLabels {
    std::vector<std::string> Rooms;
    std::vector<std::string> Floors;
};

struct DofDungeon {
    std::string Signature;            // 12 chars
    uint32_t BlockOffset = 0;
    uint32_t LinkOffset = 0;
    uint32_t GridOffset = 0;
    uint32_t GroupOffset = 0;
    uint32_t LabelOffset = 0;
    uint32_t Offset5 = 0;
    uint32_t Offset6 = 0;
    uint32_t BoundingBoxOffset = 0;

    // Info
    uint32_t Type = 0;
    std::string Name;
    uint32_t UnkUInt0 = 0, UnkUInt1 = 0;
    uint16_t RegionID = 0;

    // BoundingBox
    float CollisionBox0[24] = {0};
    float CollisionBox1[24] = {0};

    // Sections
    std::vector<DofBlock> Blocks;
    std::vector<DofVoxel> Voxels;     // only stored voxels (with block indices)
    uint32_t GridWidth = 0, GridHeight = 0, GridLength = 0;
    std::vector<DofLink> Links;
    std::vector<DofGroup> Groups;
    DofLabels Labels;

    static constexpr float kVoxelSize = 200.0f;  // units per voxel edge
};

} // namespace sro::formats
