#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include "core/Math.h"

struct BsrAniGroup {
    std::string GroupName;
    std::vector<uint32_t> AnimationIndices;
};

struct BsrMeshGroup {
    std::string BoneName;
    std::vector<uint32_t> MeshIndices;
};

struct BsrParticleAttachment {
    std::string Path;
    std::string BoneName;
    Vector3 Position;
    Vector3 Rotation;
};

class BsrResource {
public:
    std::string Signature;
    std::vector<std::string> MeshPaths;
    std::vector<std::string> MaterialPaths;
    std::string SkeletonPath;
    std::vector<std::string> AnimationPaths;
    std::vector<BsrMeshGroup> MeshGroups;
    std::vector<BsrAniGroup> AniGroups;
    std::string CollisionPath;                 // collisionMesh (.bms with NavMeshOffset)
    std::array<float, 24> CollisionBox0{};     // authored collision box 0 (raw)
    std::array<float, 24> CollisionBox1{};     // authored collision box 1 (raw, unused in dungeon nav)
    uint32_t RequireCollisionMatrix = 0;
    std::array<uint8_t, 64> CollisionMatrix{}; // 4x4 matrix when RequireCollisionMatrix
    std::vector<std::string> EffectPaths;
    std::vector<BsrParticleAttachment> ParticleAttachments;

    bool Read(const std::wstring& path);

private:
    static void ScanEmbeddedBanPaths(const std::wstring& path, std::vector<std::string>& out);
    static void ScanEmbeddedEffectPaths(const std::wstring& path, std::vector<std::string>& out);
};
