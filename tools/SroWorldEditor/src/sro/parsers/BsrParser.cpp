#include "BsrParser.h"
#include "ParserHelpers.h"
#include <fstream>

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

    return true;
}
