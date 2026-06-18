#include "BmsParser.h"
#include "ParserHelpers.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <vector>

namespace {

class MemReader {
public:
    MemReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    bool ReadBytes(void* dst, size_t count) {
        if (m_pos + count > m_size) return false;
        if (dst) std::memcpy(dst, m_data + m_pos, count);
        m_pos += count;
        return true;
    }

    bool Skip(size_t count) {
        if (m_pos + count > m_size) return false;
        m_pos += count;
        return true;
    }

    template<typename T>
    bool Read(T& out) { return ReadBytes(&out, sizeof(T)); }

    size_t Position() const { return m_pos; }

private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_pos = 0;
};

bool MeasureNavMeshSection(MemReader& r, uint32_t navFlag, size_t& outBytes) {
    const size_t start = r.Position();

    uint32_t navVertexCount = 0;
    if (!r.Read(navVertexCount) || navVertexCount > 1000000) return false;
    for (uint32_t i = 0; i < navVertexCount; ++i) {
        float x, y, z;
        uint8_t bisector;
        if (!r.Read(x) || !r.Read(y) || !r.Read(z) || !r.Read(bisector)) return false;
    }

    uint32_t cellCount = 0;
    if (!r.Read(cellCount) || cellCount > 1000000) return false;
    for (uint32_t i = 0; i < cellCount; ++i) {
        uint16_t v0, v1, v2, flag;
        if (!r.Read(v0) || !r.Read(v1) || !r.Read(v2) || !r.Read(flag)) return false;
        if (navFlag & 2) {
            uint8_t eventZone;
            if (!r.Read(eventZone)) return false;
        }
    }

    uint32_t outlineCount = 0;
    if (!r.Read(outlineCount) || outlineCount > 1000000) return false;
    for (uint32_t i = 0; i < outlineCount; ++i) {
        uint16_t sv, dv, sc, dc;
        uint8_t flag;
        if (!r.Read(sv) || !r.Read(dv) || !r.Read(sc) || !r.Read(dc) || !r.Read(flag)) return false;
        if (navFlag & 1) {
            uint8_t eventZone;
            if (!r.Read(eventZone)) return false;
        }
    }

    uint32_t inlineCount = 0;
    if (!r.Read(inlineCount) || inlineCount > 1000000) return false;
    for (uint32_t i = 0; i < inlineCount; ++i) {
        uint16_t sv, dv, sc, dc;
        uint8_t flag;
        if (!r.Read(sv) || !r.Read(dv) || !r.Read(sc) || !r.Read(dc) || !r.Read(flag)) return false;
        if (navFlag & 1) {
            uint8_t eventZone;
            if (!r.Read(eventZone)) return false;
        }
    }

    if (navFlag & 4) {
        uint32_t eventCount = 0;
        if (!r.Read(eventCount) || eventCount > 1000000) return false;
        for (uint32_t i = 0; i < eventCount; ++i) {
            uint32_t nameLen = 0;
            if (!r.Read(nameLen) || nameLen > 4096) return false;
            if (!r.Skip(nameLen)) return false;
        }
    }

    float originX, originY;
    uint32_t width, height, cellCountGrid;
    if (!r.Read(originX) || !r.Read(originY) || !r.Read(width) || !r.Read(height) || !r.Read(cellCountGrid))
        return false;
    if (cellCountGrid > 1000000) return false;
    for (uint32_t i = 0; i < cellCountGrid; ++i) {
        uint32_t cnt = 0;
        if (!r.Read(cnt) || cnt > 1000000) return false;
        if (!r.Skip(static_cast<size_t>(cnt) * sizeof(uint16_t))) return false;
    }

    outBytes = r.Position() - start;
    return true;
}

void SerializeNavMeshSection(const BmsNavMesh& nav, uint32_t navFlag, std::vector<uint8_t>& out) {
    auto append = [&](const void* p, size_t n) {
        const auto* b = static_cast<const uint8_t*>(p);
        out.insert(out.end(), b, b + n);
    };
    auto writePod = [&](const auto& v) { append(&v, sizeof(v)); };

    const uint32_t navVertexCount = static_cast<uint32_t>(nav.Vertices.size());
    writePod(navVertexCount);
    for (const auto& v : nav.Vertices) {
        writePod(v.Position.x);
        writePod(v.Position.y);
        writePod(v.Position.z);
        writePod(v.BisectorIndex);
    }

    const uint32_t cellCount = static_cast<uint32_t>(nav.Cells.size());
    writePod(cellCount);
    for (const auto& cell : nav.Cells) {
        writePod(cell.V0);
        writePod(cell.V1);
        writePod(cell.V2);
        writePod(cell.Flag);
        if (navFlag & 2) writePod(cell.EventZoneData);
    }

    const uint32_t outlineCount = static_cast<uint32_t>(nav.OutlineEdges.size());
    writePod(outlineCount);
    for (const auto& e : nav.OutlineEdges) {
        writePod(e.SrcVertex);
        writePod(e.DstVertex);
        writePod(e.SrcCell);
        writePod(e.DstCell);
        writePod(e.Flag);
        if (navFlag & 1) writePod(e.EventZoneData);
    }

    const uint32_t inlineCount = static_cast<uint32_t>(nav.InlineEdges.size());
    writePod(inlineCount);
    for (const auto& e : nav.InlineEdges) {
        writePod(e.SrcVertex);
        writePod(e.DstVertex);
        writePod(e.SrcCell);
        writePod(e.DstCell);
        writePod(e.Flag);
        if (navFlag & 1) writePod(e.EventZoneData);
    }

    if (navFlag & 4) {
        const uint32_t eventCount = static_cast<uint32_t>(nav.Events.size());
        writePod(eventCount);
        for (const auto& ev : nav.Events) {
            const uint32_t nameLen = static_cast<uint32_t>(ev.Name.size());
            writePod(nameLen);
            if (nameLen > 0) append(ev.Name.data(), nameLen);
        }
    }

    writePod(nav.OutlineGrid.Origin.x);
    writePod(nav.OutlineGrid.Origin.y);
    writePod(nav.OutlineGrid.Width);
    writePod(nav.OutlineGrid.Height);
    writePod(nav.OutlineGrid.CellCount);
    for (const auto& indices : nav.OutlineGrid.OutlineIndices) {
        const uint32_t cnt = static_cast<uint32_t>(indices.size());
        writePod(cnt);
        for (uint16_t idx : indices) writePod(idx);
    }
}

} // namespace

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

    if (skinOffset > 0 && !Vertices.empty()) {
        fs.seekg(skinOffset, std::ios::beg);
        uint32_t boneCount = 0;
        ReadVal(fs, boneCount);
        if (fs && boneCount > 0 && boneCount < 512) {
            SkinBones.resize(boneCount);
            for (uint32_t i = 0; i < boneCount; ++i) {
                uint32_t boneNameLen = 0;
                ReadVal(fs, boneNameLen);
                if (!fs || boneNameLen > 128) return false;
                SkinBones[i] = ReadString(fs, boneNameLen);
            }
            Skin.resize(Vertices.size());
            for (size_t vi = 0; vi < Vertices.size(); ++vi) {
                uint8_t idx1 = 0, idx2 = 0;
                uint16_t w1 = 0, w2 = 0;
                ReadVal(fs, idx1);
                ReadVal(fs, w1);
                ReadVal(fs, idx2);
                ReadVal(fs, w2);
                if (!fs) return false;
                const float fw1 = static_cast<float>(w1);
                const float fw2 = static_cast<float>(w2);
                const float sum = fw1 + fw2;
                if (sum > 0.f) {
                    Skin[vi].boneIdx1 = idx1;
                    Skin[vi].boneIdx2 = idx2;
                    Skin[vi].weight1 = fw1 / sum;
                    Skin[vi].weight2 = fw2 / sum;
                }
            }
        }
    }

    // NavMeshOffset -> RTNavMeshObj (the per-object navmesh / collision geometry).
    // See SilkroadDoc JMXVBMS. This is what JMXVNVM ObjectList instances reference.
    if (navMeshOffset > 0) {
        fs.seekg(navMeshOffset, std::ios::beg);
        NavMesh.HasData = true;
        NavMesh.NavFlag = navFlag;

        // NavVertices
        uint32_t navVertexCount = 0;
        ReadVal(fs, navVertexCount);
        if (!fs || navVertexCount > 1000000) return false;
        NavMesh.Vertices.resize(navVertexCount);
        for (uint32_t i = 0; i < navVertexCount; ++i) {
            ReadVal(fs, NavMesh.Vertices[i].Position.x);
            ReadVal(fs, NavMesh.Vertices[i].Position.y);
            ReadVal(fs, NavMesh.Vertices[i].Position.z);
            ReadVal(fs, NavMesh.Vertices[i].BisectorIndex);
            if (!fs) return false;
        }

        // NavCells (ObjectGround) -- RTNavMeshCellTri
        uint32_t cellCount = 0;
        ReadVal(fs, cellCount);
        if (!fs || cellCount > 1000000) return false;
        NavMesh.Cells.resize(cellCount);
        for (uint32_t i = 0; i < cellCount; ++i) {
            ReadVal(fs, NavMesh.Cells[i].V0);
            ReadVal(fs, NavMesh.Cells[i].V1);
            ReadVal(fs, NavMesh.Cells[i].V2);
            ReadVal(fs, NavMesh.Cells[i].Flag);
            if (!fs) return false;
            if (navFlag & 2) {
                ReadVal(fs, NavMesh.Cells[i].EventZoneData);
                if (!fs) return false;
            }
        }

        // NavOutlineEdges
        uint32_t outlineCount = 0;
        ReadVal(fs, outlineCount);
        if (!fs || outlineCount > 1000000) return false;
        NavMesh.OutlineEdges.resize(outlineCount);
        for (uint32_t i = 0; i < outlineCount; ++i) {
            ReadVal(fs, NavMesh.OutlineEdges[i].SrcVertex);
            ReadVal(fs, NavMesh.OutlineEdges[i].DstVertex);
            ReadVal(fs, NavMesh.OutlineEdges[i].SrcCell);
            ReadVal(fs, NavMesh.OutlineEdges[i].DstCell);
            ReadVal(fs, NavMesh.OutlineEdges[i].Flag);
            if (!fs) return false;
            if (navFlag & 1) {
                ReadVal(fs, NavMesh.OutlineEdges[i].EventZoneData);
                if (!fs) return false;
            }
        }

        // NavInlineEdges
        uint32_t inlineCount = 0;
        ReadVal(fs, inlineCount);
        if (!fs || inlineCount > 1000000) return false;
        NavMesh.InlineEdges.resize(inlineCount);
        for (uint32_t i = 0; i < inlineCount; ++i) {
            ReadVal(fs, NavMesh.InlineEdges[i].SrcVertex);
            ReadVal(fs, NavMesh.InlineEdges[i].DstVertex);
            ReadVal(fs, NavMesh.InlineEdges[i].SrcCell);
            ReadVal(fs, NavMesh.InlineEdges[i].DstCell);
            ReadVal(fs, NavMesh.InlineEdges[i].Flag);
            if (!fs) return false;
            if (navFlag & 1) {
                ReadVal(fs, NavMesh.InlineEdges[i].EventZoneData);
                if (!fs) return false;
            }
        }

        // Events (only when NavFlag & 4)
        if (navFlag & 4) {
            uint32_t eventCount = 0;
            ReadVal(fs, eventCount);
            if (!fs || eventCount > 1000000) return false;
            NavMesh.Events.resize(eventCount);
            for (uint32_t i = 0; i < eventCount; ++i) {
                uint32_t nameLen = 0;
                ReadVal(fs, nameLen);
                if (!fs || nameLen > 4096) return false;
                NavMesh.Events[i].Name = ReadString(fs, nameLen);
            }
        }

        // OutlineLookupGrid
        ReadVal(fs, NavMesh.OutlineGrid.Origin.x);
        ReadVal(fs, NavMesh.OutlineGrid.Origin.y);
        ReadVal(fs, NavMesh.OutlineGrid.Width);
        ReadVal(fs, NavMesh.OutlineGrid.Height);
        ReadVal(fs, NavMesh.OutlineGrid.CellCount);
        if (!fs) return false;
        if (NavMesh.OutlineGrid.CellCount > 1000000) return false;
        NavMesh.OutlineGrid.OutlineIndices.resize(NavMesh.OutlineGrid.CellCount);
        for (uint32_t i = 0; i < NavMesh.OutlineGrid.CellCount; ++i) {
            uint32_t cnt = 0;
            ReadVal(fs, cnt);
            if (!fs || cnt > 1000000) return false;
            NavMesh.OutlineGrid.OutlineIndices[i].resize(cnt);
            for (uint32_t j = 0; j < cnt; ++j) {
                ReadVal(fs, NavMesh.OutlineGrid.OutlineIndices[i][j]);
                if (!fs) return false;
            }
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

bool WriteNavMeshOffset(const std::wstring& path, const BmsNavMesh& nav) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;

    in.seekg(0, std::ios::end);
    const auto fileSize = in.tellg();
    if (fileSize <= 0) return false;
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> file(static_cast<size_t>(fileSize));
    in.read(reinterpret_cast<char*>(file.data()), fileSize);
    if (!in) return false;
    in.close();

    if (file.size() < 56) return false;
    if (std::string(reinterpret_cast<char*>(file.data()), 7) != "JMXVBMS") return false;

    uint32_t navMeshOffset = 0;
    std::memcpy(&navMeshOffset, file.data() + 40, sizeof(navMeshOffset));
    if (navMeshOffset == 0 || navMeshOffset >= file.size()) return false;

    uint32_t navFlag = 0;
    std::memcpy(&navFlag, file.data() + 56, sizeof(navFlag));

    MemReader measureReader(file.data() + navMeshOffset, file.size() - navMeshOffset);
    size_t oldSectionSize = 0;
    if (!MeasureNavMeshSection(measureReader, navFlag, oldSectionSize)) return false;
    if (navMeshOffset + oldSectionSize > file.size()) return false;

    std::vector<uint8_t> newSection;
    SerializeNavMeshSection(nav, navFlag, newSection);

    std::vector<uint8_t> out;
    out.reserve(file.size() - oldSectionSize + newSection.size());
    out.insert(out.end(), file.begin(), file.begin() + navMeshOffset);
    out.insert(out.end(), newSection.begin(), newSection.end());
    out.insert(out.end(), file.begin() + navMeshOffset + oldSectionSize, file.end());

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(ofs);
}
