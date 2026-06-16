#include "BmsParser.h"
#include "ParserHelpers.h"
#include <fstream>
#include <algorithm>

bool BmsMesh::Read(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    char sig[13] = {0};
    fs.read(sig, 12);
    Signature = sig;
    if (Signature.rfind("JMXVBMS", 0) != 0) return false;

    uint32_t vertexOffset = 0, skinOffset = 0, faceOffset = 0, clothVertexOffset = 0, clothEdgeOffset = 0;
    uint32_t boundingBoxOffset = 0, occlusionPortals = 0, navMeshOffset = 0, skinedNavMeshOffset = 0, unknown9Offset = 0;
    ReadVal(fs, vertexOffset);
    ReadVal(fs, skinOffset);
    ReadVal(fs, faceOffset);
    ReadVal(fs, clothVertexOffset);
    ReadVal(fs, clothEdgeOffset);
    ReadVal(fs, boundingBoxOffset);
    ReadVal(fs, occlusionPortals);
    ReadVal(fs, navMeshOffset);
    ReadVal(fs, skinedNavMeshOffset);
    ReadVal(fs, unknown9Offset);
    if (!fs) return false;

    uint32_t unkUInt0 = 0, navFlag = 0, subPrimCount = 0, vertexFlag = 0, unkUInt2 = 0;
    ReadVal(fs, unkUInt0);
    ReadVal(fs, navFlag);
    ReadVal(fs, subPrimCount);
    ReadVal(fs, vertexFlag);
    ReadVal(fs, unkUInt2);
    if (!fs) return false;

    uint32_t nameLen = 0;
    ReadVal(fs, nameLen);
    if (!fs) return false;
    if (nameLen > 0) fs.seekg(nameLen, std::ios::cur);

    uint32_t matLen = 0;
    ReadVal(fs, matLen);
    if (!fs) return false;
    if (matLen > 0) {
        Material = ReadString(fs, matLen);
    }
    if (!fs) return false;

    uint32_t unkUInt3 = 0;
    ReadVal(fs, unkUInt3);
    if (!fs) return false;

    if (vertexOffset > 0) {
        fs.seekg(vertexOffset, std::ios::beg);
        uint32_t vertexCount = 0;
        ReadVal(fs, vertexCount);
        if (!fs) return false;
        if (vertexCount > 1000000) return false;
        Vertices.resize(vertexCount);
        for (uint32_t i = 0; i < vertexCount; ++i) {
            ReadVal(fs, Vertices[i].X);
            ReadVal(fs, Vertices[i].Y);
            ReadVal(fs, Vertices[i].Z);
            ReadVal(fs, Vertices[i].NX);
            ReadVal(fs, Vertices[i].NY);
            ReadVal(fs, Vertices[i].NZ);
            ReadVal(fs, Vertices[i].U);
            ReadVal(fs, Vertices[i].V);

            if (vertexFlag & 0x400) {
                float uv1_u = 0, uv1_v = 0;
                ReadVal(fs, uv1_u);
                ReadVal(fs, uv1_v);
            }
            if (vertexFlag & 0x800) {
                fs.seekg(36, std::ios::cur);
            }
            float f0 = 0;
            uint32_t i0 = 0, i1 = 0;
            ReadVal(fs, f0);
            ReadVal(fs, i0);
            ReadVal(fs, i1);
            if (!fs) return false;
        }
    }

    if (faceOffset > 0) {
        fs.seekg(faceOffset, std::ios::beg);
        uint32_t faceCount = 0;
        ReadVal(fs, faceCount);
        if (!fs) return false;
        if (faceCount > 1000000) return false;
        Faces.resize(faceCount);
        for (uint32_t i = 0; i < faceCount; ++i) {
            ReadVal(fs, Faces[i].A);
            ReadVal(fs, Faces[i].B);
            ReadVal(fs, Faces[i].C);
            if (!fs) return false;
        }
    }

    return true;
}

std::vector<float> BmsMesh::GetFlattenedVertices() const {
    std::vector<float> data;
    data.reserve(Vertices.size() * 8);
    for (const auto& v : Vertices) {
        data.push_back(v.X);
        data.push_back(v.Y);
        data.push_back(v.Z);
        data.push_back(v.NX);
        data.push_back(v.NY);
        data.push_back(v.NZ);
        data.push_back(v.U);
        data.push_back(v.V);
    }
    return data;
}

std::vector<uint32_t> BmsMesh::GetFlattenedIndices() const {
    std::vector<uint32_t> data;
    data.reserve(Faces.size() * 3);
    for (const auto& f : Faces) {
        data.push_back(f.A);
        data.push_back(f.B);
        data.push_back(f.C);
    }
    return data;
}
