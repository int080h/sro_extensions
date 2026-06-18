#include "BsrParser.h"
#include "ParserHelpers.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <unordered_set>

namespace {

bool IsPrintablePathChar(unsigned char c) {
    return c >= 32 && c < 127;
}

std::vector<std::string> ScanLengthPrefixedStrings(const std::vector<uint8_t>& data) {
    std::vector<std::string> strings;
    for (size_t i = 0; i + sizeof(uint32_t) <= data.size(); ++i) {
        uint32_t len = 0;
        std::memcpy(&len, data.data() + i, sizeof(len));
        if (len < 4 || len > 260 || i + sizeof(uint32_t) + len > data.size()) continue;

        const uint8_t* start = data.data() + i + sizeof(uint32_t);
        bool printable = true;
        for (uint32_t j = 0; j < len; ++j) {
            if (!IsPrintablePathChar(start[j])) {
                printable = false;
                break;
            }
        }
        if (!printable) continue;

        strings.emplace_back(reinterpret_cast<const char*>(start), len);
    }
    return strings;
}

} // namespace

static bool IsValidBanPath(const std::string& s) {
    if (s.size() < 5) return false;
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.size() < 4 || lower.compare(lower.size() - 4, 4, ".ban") != 0) return false;
    for (char c : s) {
        if (c == '\0') return false;
    }
    return true;
}

static bool IsValidEffectPath(const std::string& s) {
    if (s.size() < 5 || s.size() > 260) return false;
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.size() < 4 || lower.compare(lower.size() - 4, 4, ".efp") != 0) return false;
    for (char c : s) {
        if (c == '\0') return false;
    }
    return true;
}

void BsrResource::ScanEmbeddedBanPaths(const std::wstring& path, std::vector<std::string>& out) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    std::unordered_set<std::string> seen;
    for (const std::string& s : ScanLengthPrefixedStrings(data)) {
        if (!IsValidBanPath(s)) continue;
        if (seen.insert(s).second) out.push_back(s);
    }
}

void BsrResource::ScanEmbeddedEffectPaths(const std::wstring& path, std::vector<std::string>& out) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    std::unordered_set<std::string> seen;
    for (const std::string& s : ScanLengthPrefixedStrings(data)) {
        if (!IsValidEffectPath(s)) continue;
        if (seen.insert(s).second) out.push_back(s);
    }
}

bool BsrResource::Read(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    char sig[13] = {0};
    fs.read(sig, 12);
    Signature = sig;
    if (Signature.rfind("JMXVRES", 0) != 0) return false;

    uint32_t materialOffset = 0, meshOffset = 0, skeletonOffset = 0, animationOffset = 0;
    uint32_t primMeshGroupOffset = 0, primAniGroupOffset = 0, modPaletteOffset = 0, collisionOffset = 0;
    uint32_t primMeshFlag = 0, modDataFlag = 0, int2 = 0, int3 = 0, int4 = 0;

    ReadVal(fs, materialOffset);
    ReadVal(fs, meshOffset);
    ReadVal(fs, skeletonOffset);
    ReadVal(fs, animationOffset);
    ReadVal(fs, primMeshGroupOffset);
    ReadVal(fs, primAniGroupOffset);
    ReadVal(fs, modPaletteOffset);
    ReadVal(fs, collisionOffset);
    ReadVal(fs, primMeshFlag);
    ReadVal(fs, modDataFlag);
    ReadVal(fs, int2);
    ReadVal(fs, int3);
    ReadVal(fs, int4);
    if (!fs) return false;

    if (meshOffset > 0) {
        fs.seekg(meshOffset, std::ios::beg);
        uint32_t meshCnt = 0;
        ReadVal(fs, meshCnt);
        if (!fs) return false;
        if (meshCnt > 10000) return false;
        MeshPaths.resize(meshCnt);
        for (uint32_t i = 0; i < meshCnt; ++i) {
            uint32_t pathLen = 0;
            ReadVal(fs, pathLen);
            if (!fs) return false;
            MeshPaths[i] = ReadString(fs, pathLen);

            if (primMeshFlag & 1) {
                fs.seekg(4, std::ios::cur);
            }
            if (!fs) return false;
        }
    }

    if (materialOffset > 0) {
        fs.seekg(materialOffset, std::ios::beg);
        uint32_t mtrlSetCnt = 0;
        ReadVal(fs, mtrlSetCnt);
        if (!fs) return false;
        if (mtrlSetCnt > 10000) return false;
        MaterialPaths.resize(mtrlSetCnt);
        for (uint32_t i = 0; i < mtrlSetCnt; ++i) {
            uint32_t mtrlID = 0;
            ReadVal(fs, mtrlID);
            uint32_t pathLen = 0;
            ReadVal(fs, pathLen);
            if (!fs) return false;
            MaterialPaths[i] = ReadString(fs, pathLen);
        }
    }

    if (skeletonOffset > 0) {
        fs.seekg(skeletonOffset, std::ios::beg);
        uint32_t hasSkeleton = 0;
        ReadVal(fs, hasSkeleton);
        if (hasSkeleton == 1) {
            uint32_t pathLen = 0;
            ReadVal(fs, pathLen);
            if (fs) {
                SkeletonPath = ReadString(fs, pathLen);
            }
        }
    }

    if (animationOffset > 0) {
        fs.seekg(animationOffset, std::ios::beg);
        uint32_t animationTypeVersion = 0;
        uint32_t animationTypeUserDefine = 0;
        ReadVal(fs, animationTypeVersion);
        ReadVal(fs, animationTypeUserDefine);
        uint32_t animCnt = 0;
        ReadVal(fs, animCnt);
        if (fs && animCnt > 0 && animCnt < 2000) {
            AnimationPaths.reserve(animCnt);
            for (uint32_t i = 0; i < animCnt; ++i) {
                uint32_t pathLen = 0;
                ReadVal(fs, pathLen);
                if (!fs || pathLen == 0 || pathLen > 512) break;
                std::string animPath = ReadString(fs, pathLen);
                if (IsValidBanPath(animPath)) {
                    AnimationPaths.push_back(animPath);
                }
            }
        }
    }

    if (primMeshGroupOffset > 0) {
        fs.seekg(primMeshGroupOffset, std::ios::beg);
        uint32_t meshGroupCnt = 0;
        ReadVal(fs, meshGroupCnt);
        if (fs && meshGroupCnt < 10000) {
            MeshGroups.resize(meshGroupCnt);
            for (uint32_t i = 0; i < meshGroupCnt; ++i) {
                uint32_t nameLen = 0;
                ReadVal(fs, nameLen);
                MeshGroups[i].BoneName = ReadString(fs, nameLen);
                uint32_t meshFileCnt = 0;
                ReadVal(fs, meshFileCnt);
                if (fs && meshFileCnt < 1000) {
                    MeshGroups[i].MeshIndices.resize(meshFileCnt);
                    for (uint32_t ii = 0; ii < meshFileCnt; ++ii) {
                        ReadVal(fs, MeshGroups[i].MeshIndices[ii]);
                    }
                }
            }
        }
    }

    if (primAniGroupOffset > 0) {
        fs.seekg(primAniGroupOffset, std::ios::beg);
        uint32_t aniGroupCnt = 0;
        ReadVal(fs, aniGroupCnt);
        if (fs && aniGroupCnt > 0 && aniGroupCnt < 256) {
            AniGroups.resize(aniGroupCnt);
            for (uint32_t i = 0; i < aniGroupCnt; ++i) {
                uint32_t nameLen = 0;
                ReadVal(fs, nameLen);
                if (!fs || nameLen > 128) break;
                AniGroups[i].GroupName = ReadString(fs, nameLen);
                uint32_t idxCnt = 0;
                ReadVal(fs, idxCnt);
                if (!fs || idxCnt > 512) break;
                for (uint32_t j = 0; j < idxCnt; ++j) {
                    uint32_t animationType = 0;
                    uint32_t fileIndex = 0;
                    ReadVal(fs, animationType);
                    ReadVal(fs, fileIndex);
                    if (!fs) break;
                    if (fileIndex < AnimationPaths.size())
                        AniGroups[i].AnimationIndices.push_back(fileIndex);

                    uint32_t eventCnt = 0;
                    ReadVal(fs, eventCnt);
                    if (!fs || eventCnt > 4096) break;
                    fs.seekg(static_cast<std::streamoff>(eventCnt) * 16, std::ios::cur);

                    uint32_t walkPointCnt = 0;
                    float walkLength = 0.f;
                    ReadVal(fs, walkPointCnt);
                    ReadVal(fs, walkLength);
                    if (!fs || walkPointCnt > 4096) break;
                    fs.seekg(static_cast<std::streamoff>(walkPointCnt) * 8, std::ios::cur);
                }
            }
        }
    }

    if (collisionOffset > 0) {
        // JMXVRES CollisionOffset (per spec, NO hasCollision flag):
        //   uint collisionMesh.Length; string collisionMesh;
        //   float[24] collisionBox0; float[24] collisionBox1;
        //   uint requireCollisionMatrix; if (requireCollisionMatrix) byte[64] collisionMatrix;
        // The original code misread the length as a "hasCollision" flag, which
        // silently dropped the collision path for every BSR (length is a real path
        // length like 70, not 0/1). Fixed to match the documented format exactly.
        fs.seekg(collisionOffset, std::ios::beg);
        uint32_t meshLen = 0;
        ReadVal(fs, meshLen);
        if (fs && meshLen > 0 && meshLen < 512) {
            CollisionPath = ReadString(fs, meshLen);
            for (int i = 0; i < 24; ++i) { ReadVal(fs, CollisionBox0[i]); }
            for (int i = 0; i < 24; ++i) { ReadVal(fs, CollisionBox1[i]); }
            ReadVal(fs, RequireCollisionMatrix);
            if (RequireCollisionMatrix) {
                fs.read(reinterpret_cast<char*>(CollisionMatrix.data()), 64);
            }
            if (!fs) {
                CollisionBox0.fill(0.0f);
                CollisionBox1.fill(0.0f);
                RequireCollisionMatrix = 0;
                fs.clear();
            }
        }
    }

    if (modPaletteOffset > 0) {
        fs.seekg(modPaletteOffset, std::ios::beg);
        uint32_t modSetCnt = 0;
        ReadVal(fs, modSetCnt);
        if (fs && modSetCnt < 1000) {
            for (uint32_t i = 0; i < modSetCnt; ++i) {
                uint32_t modSetType = 0;
                uint32_t modSetAniType = 0;
                ReadVal(fs, modSetType);
                ReadVal(fs, modSetAniType);
                uint32_t modSetNameLen = 0;
                ReadVal(fs, modSetNameLen);
                if (modSetNameLen > 512) break;
                std::string modSetName = ReadString(fs, modSetNameLen);

                uint32_t modSetDataCnt = 0;
                ReadVal(fs, modSetDataCnt);
                if (modSetDataCnt > 1000) break;
                for (uint32_t j = 0; j < modSetDataCnt; ++j) {
                    uint32_t modDataType = 0;
                    ReadVal(fs, modDataType);
                    if (modDataType == 0x30000) {
                        float unkFloat1 = 0.0f;
                        uint32_t unkInt2 = 0, unkInt3 = 0;
                        int32_t unkInt4 = 0;
                        float f1 = 0.0f, f2 = 0.0f, f3 = 0.0f;
                        uint32_t count = 0, unkInt6 = 0;
                        ReadVal(fs, unkFloat1);
                        ReadVal(fs, unkInt2);
                        ReadVal(fs, unkInt3);
                        ReadVal(fs, unkInt4);
                        ReadVal(fs, f1); ReadVal(fs, f2); ReadVal(fs, f3);
                        ReadVal(fs, count);
                        ReadVal(fs, unkInt6);

                        if (fs && count < 1000) {
                            for (uint32_t k = 0; k < count; ++k) {
                                uint32_t pathLen = 0;
                                ReadVal(fs, pathLen);
                                if (pathLen > 512) break;
                                std::string pathStr = ReadString(fs, pathLen);

                                uint32_t boneLen = 0;
                                ReadVal(fs, boneLen);
                                if (boneLen > 512) break;
                                std::string boneName = ReadString(fs, boneLen);

                                Vector3 position;
                                ReadVal(fs, position.x);
                                ReadVal(fs, position.y);
                                ReadVal(fs, position.z);

                                Vector3 rotation;
                                ReadVal(fs, rotation.x);
                                ReadVal(fs, rotation.y);
                                ReadVal(fs, rotation.z);



                                BsrParticleAttachment attachment;
                                attachment.Path = pathStr;
                                attachment.BoneName = boneName;
                                attachment.Position = position;
                                attachment.Rotation = rotation;
                                ParticleAttachments.push_back(attachment);
                            }
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    }

    if (AnimationPaths.empty()) {
        ScanEmbeddedBanPaths(path, AnimationPaths);
    }
    ScanEmbeddedEffectPaths(path, EffectPaths);

    return true;
}
