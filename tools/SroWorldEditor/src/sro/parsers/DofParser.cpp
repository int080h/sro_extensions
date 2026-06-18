#include "DofParser.h"
#include "io/BinaryReader.h"
#include <cstring>
#include <vector>

namespace sro {

namespace {

bool ReadSizedString(BinaryReader& r, std::string& out) {
    uint32_t len = 0;
    if (!r.Read(len)) return false;
    if (len > (1u << 20)) return false;
    out.assign(len, '\0');
    if (len > 0) {
        if (!r.ReadBytes(out.data(), len)) return false;
        while (!out.empty() && (out.back() == '\0' || out.back() == '\r' || out.back() == '\n'))
            out.pop_back();
    }
    return true;
}

bool ReadFog(BinaryReader& r, formats::DofFogParam& fog) {
    if (!r.Read(fog.Color)) return false;
    if (!r.Read(fog.NearPlane)) return false;
    if (!r.Read(fog.FarPlane)) return false;
    if (!r.Read(fog.Intensity)) return false;
    if (!r.Read(fog.HasHeightFog)) return false;
    if (fog.HasHeightFog == 0x01) {
        for (int i = 0; i < 4; ++i)
            if (!r.Read(fog.HeightFog[i])) return false;
    }
    return true;
}

bool ReadBlock(BinaryReader& r, formats::DofBlock& b) {
    if (!ReadSizedString(r, b.Path)) return false;
    if (!ReadSizedString(r, b.Name)) return false;
    if (!r.Read(b.UnkUInt0)) return false;
    if (!r.Read(b.Position.x)) return false;
    if (!r.Read(b.Position.y)) return false;
    if (!r.Read(b.Position.z)) return false;
    if (!r.Read(b.Yaw)) return false;
    if (!r.Read(b.IsEntrance)) return false;
    for (int i = 0; i < 24; ++i) if (!r.Read(b.CollisionBox0[i])) return false;
    if (!r.Read(b.UnkUInt1)) return false;
    if (!ReadFog(r, b.Fog)) return false;
    if (!r.Read(b.UnkByte1)) return false;
    if (b.UnkByte1 == 0x02) {
        if (!r.Read(b.UnkVector0.x) || !r.Read(b.UnkVector0.y) || !r.Read(b.UnkVector0.z)) return false;
        if (!r.Read(b.UnkVector1.x) || !r.Read(b.UnkVector1.y) || !r.Read(b.UnkVector1.z)) return false;
        if (!r.Read(b.UnkUInt2)) return false;
    }
    if (!ReadSizedString(r, b.UnkString)) return false;
    if (!r.Read(b.RoomIndex)) return false;
    if (!r.Read(b.FloorIndex)) return false;

    uint32_t cnt = 0;
    if (!r.Read(cnt) || cnt > 100000) return false;
    b.ConnectedBlockIndices.resize(cnt);
    for (uint32_t i = 0; i < cnt; ++i) if (!r.Read(b.ConnectedBlockIndices[i])) return false;

    if (!r.Read(cnt) || cnt > 100000) return false;
    b.VisibleBlockIndices.resize(cnt);
    for (uint32_t i = 0; i < cnt; ++i) if (!r.Read(b.VisibleBlockIndices[i])) return false;

    uint32_t objCount = 0, colObjCount = 0;
    if (!r.Read(objCount) || objCount > 100000) return false;
    if (!r.Read(colObjCount) || colObjCount > 100000) return false;
    b.Objects.resize(objCount);
    for (uint32_t i = 0; i < objCount; ++i) {
        auto& o = b.Objects[i];
        if (!ReadSizedString(r, o.Name)) return false;
        if (!ReadSizedString(r, o.Path)) return false;
        if (!r.Read(o.Position.x) || !r.Read(o.Position.y) || !r.Read(o.Position.z)) return false;
        if (!r.Read(o.Rotation.x) || !r.Read(o.Rotation.y) || !r.Read(o.Rotation.z)) return false;
        if (!r.Read(o.Scale.x) || !r.Read(o.Scale.y) || !r.Read(o.Scale.z)) return false;
        if (!r.Read(o.Flag)) return false;
        if (!r.Read(o.Int0)) return false;
        if (!r.Read(o.RadiusSqrt)) return false;
        if (o.Flag & 4) { if (!r.Read(o.WaterColor)) return false; }
    }

    uint32_t lightCount = 0;
    if (!r.Read(lightCount) || lightCount > 100000) return false;
    b.Lights.resize(lightCount);
    for (uint32_t i = 0; i < lightCount; ++i) {
        auto& l = b.Lights[i];
        if (!ReadSizedString(r, l.Name)) return false;
        if (!r.Read(l.Position.x) || !r.Read(l.Position.y) || !r.Read(l.Position.z)) return false;
        for (int j = 0; j < 3; ++j) if (!r.Read(l.Color0[j])) return false;
        for (int j = 0; j < 3; ++j) if (!r.Read(l.Color1[j])) return false;
        for (int j = 0; j < 3; ++j) if (!r.Read(l.Color2[j])) return false;
        if (!r.Read(l.Float0) || !r.Read(l.Float1) || !r.Read(l.Float2)) return false;
    }
    return true;
}

} // namespace

ParseResult DofParser::Read(const std::wstring& path, formats::DofDungeon& out) {
    BinaryReader r(path);
    if (!r.IsOpen()) return ParseResult::Fail("Cannot open file", path);
    if (!r.ReadSignature(out.Signature)) return ParseResult::Fail("Failed to read signature", path);
    if (out.Signature.rfind("JMXVDOF", 0) != 0) return ParseResult::Fail("Invalid JMXVDOF signature", path);

    if (!r.Read(out.BlockOffset)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.LinkOffset)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.GridOffset)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.GroupOffset)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.LabelOffset)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.Offset5)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.Offset6)) return ParseResult::Fail("Truncated header", path);
    if (!r.Read(out.BoundingBoxOffset)) return ParseResult::Fail("Truncated header", path);

    // Info (immediately follows header)
    if (!r.Read(out.Type)) return ParseResult::Fail("Truncated info", path);
    if (!ReadSizedString(r, out.Name)) return ParseResult::Fail("Truncated info name", path);
    if (!r.Read(out.UnkUInt0)) return ParseResult::Fail("Truncated info", path);
    if (!r.Read(out.UnkUInt1)) return ParseResult::Fail("Truncated info", path);
    if (!r.Read(out.RegionID)) return ParseResult::Fail("Truncated info", path);

    // BoundingBox
    if (out.BoundingBoxOffset > 0) {
        r.Seek(out.BoundingBoxOffset);
        for (int i = 0; i < 24; ++i) if (!r.Read(out.CollisionBox0[i])) return ParseResult::Fail("Truncated bbox", path);
        for (int i = 0; i < 24; ++i) if (!r.Read(out.CollisionBox1[i])) return ParseResult::Fail("Truncated bbox", path);
    }

    // BlockList
    if (out.BlockOffset > 0) {
        r.Seek(out.BlockOffset);
        uint32_t blockCount = 0;
        if (!r.Read(blockCount) || blockCount > 100000) return ParseResult::Fail("Truncated block list", path);
        out.Blocks.resize(blockCount);
        for (uint32_t i = 0; i < blockCount; ++i) {
            if (!ReadBlock(r, out.Blocks[i])) return ParseResult::Fail("Truncated block", path);
        }
    }

    // 3D Block Lookup Grid
    if (out.GridOffset > 0) {
        r.Seek(out.GridOffset);
        if (!r.Read(out.GridWidth)) return ParseResult::Fail("Truncated grid", path);
        if (!r.Read(out.GridHeight)) return ParseResult::Fail("Truncated grid", path);
        if (!r.Read(out.GridLength)) return ParseResult::Fail("Truncated grid", path);
        uint32_t voxelCount = 0;
        if (!r.Read(voxelCount) || voxelCount > 1000000) return ParseResult::Fail("Truncated grid", path);
        out.Voxels.resize(voxelCount);
        for (uint32_t i = 0; i < voxelCount; ++i) {
            auto& v = out.Voxels[i];
            if (!r.Read(v.ID)) return ParseResult::Fail("Truncated voxel", path);
            uint32_t bc = 0;
            if (!r.Read(bc) || bc > 100000) return ParseResult::Fail("Truncated voxel", path);
            v.BlockIndices.resize(bc);
            for (uint32_t j = 0; j < bc; ++j) if (!r.Read(v.BlockIndices[j])) return ParseResult::Fail("Truncated voxel", path);
        }
    }

    // Links (off-mesh between blocks)
    if (out.LinkOffset > 0) {
        r.Seek(out.LinkOffset);
        uint32_t linkCount = 0;
        if (!r.Read(linkCount) || linkCount > 1000000) return ParseResult::Fail("Truncated links", path);
        out.Links.resize(linkCount);
        for (uint32_t i = 0; i < linkCount; ++i) {
            uint32_t bc = 0;
            if (!r.Read(bc) || bc > 100000) return ParseResult::Fail("Truncated link", path);
            out.Links[i].BlockIndices.resize(bc);
            for (uint32_t j = 0; j < bc; ++j) if (!r.Read(out.Links[i].BlockIndices[j])) return ParseResult::Fail("Truncated link", path);
        }
    }

    // Groups
    if (out.GroupOffset > 0) {
        r.Seek(out.GroupOffset);
        uint32_t groupCount = 0;
        if (!r.Read(groupCount) || groupCount > 100000) return ParseResult::Fail("Truncated groups", path);
        out.Groups.resize(groupCount);
        for (uint32_t i = 0; i < groupCount; ++i) {
            auto& g = out.Groups[i];
            if (!ReadSizedString(r, g.Name)) return ParseResult::Fail("Truncated group", path);
            if (!r.Read(g.Flag)) return ParseResult::Fail("Truncated group", path);
            uint32_t bc = 0;
            if (!r.Read(bc) || bc > 100000) return ParseResult::Fail("Truncated group", path);
            g.BlockIndices.resize(bc);
            for (uint32_t j = 0; j < bc; ++j) if (!r.Read(g.BlockIndices[j])) return ParseResult::Fail("Truncated group", path);
        }
    }

    // Labels
    if (out.LabelOffset != 0) {
        r.Seek(out.LabelOffset);
        uint32_t roomCount = 0;
        if (!r.Read(roomCount) || roomCount > 100000) return ParseResult::Fail("Truncated labels", path);
        out.Labels.Rooms.resize(roomCount);
        for (uint32_t i = 0; i < roomCount; ++i) {
            if (!ReadSizedString(r, out.Labels.Rooms[i])) return ParseResult::Fail("Truncated label", path);
        }
        uint32_t floorCount = 0;
        if (!r.Read(floorCount) || floorCount > 100000) return ParseResult::Fail("Truncated labels", path);
        out.Labels.Floors.resize(floorCount);
        for (uint32_t i = 0; i < floorCount; ++i) {
            uint16_t len = 0;
            if (!r.Read(len)) return ParseResult::Fail("Truncated label", path);
            out.Labels.Floors[i].assign(len, '\0');
            if (len > 0) {
                if (!r.ReadBytes(out.Labels.Floors[i].data(), len)) return ParseResult::Fail("Truncated label", path);
                while (!out.Labels.Floors[i].empty() && out.Labels.Floors[i].back() == '\0')
                    out.Labels.Floors[i].pop_back();
            }
        }
    }

    return ParseResult::Success(path);
}

namespace {

class VectorWriter {
public:
    explicit VectorWriter(std::vector<uint8_t>& out) : m_out(out) {}

    bool WriteBytes(const void* src, size_t count) {
        const auto* b = static_cast<const uint8_t*>(src);
        m_out.insert(m_out.end(), b, b + count);
        return true;
    }

    template<typename T>
    bool Write(const T& val) { return WriteBytes(&val, sizeof(T)); }

    uint32_t Size() const { return static_cast<uint32_t>(m_out.size()); }

private:
    std::vector<uint8_t>& m_out;
};

bool WriteSizedString(VectorWriter& w, const std::string& s) {
    const uint32_t len = static_cast<uint32_t>(s.size());
    if (!w.Write(len)) return false;
    if (len > 0 && !w.WriteBytes(s.data(), len)) return false;
    return true;
}

bool WriteFog(VectorWriter& w, const formats::DofFogParam& fog) {
    if (!w.Write(fog.Color)) return false;
    if (!w.Write(fog.NearPlane)) return false;
    if (!w.Write(fog.FarPlane)) return false;
    if (!w.Write(fog.Intensity)) return false;
    if (!w.Write(fog.HasHeightFog)) return false;
    if (fog.HasHeightFog == 0x01) {
        for (int i = 0; i < 4; ++i)
            if (!w.Write(fog.HeightFog[i])) return false;
    }
    return true;
}

bool WriteBlock(VectorWriter& w, const formats::DofBlock& b) {
    if (!WriteSizedString(w, b.Path)) return false;
    if (!WriteSizedString(w, b.Name)) return false;
    if (!w.Write(b.UnkUInt0)) return false;
    if (!w.Write(b.Position.x) || !w.Write(b.Position.y) || !w.Write(b.Position.z)) return false;
    if (!w.Write(b.Yaw)) return false;
    if (!w.Write(b.IsEntrance)) return false;
    for (int i = 0; i < 24; ++i) if (!w.Write(b.CollisionBox0[i])) return false;
    if (!w.Write(b.UnkUInt1)) return false;
    if (!WriteFog(w, b.Fog)) return false;
    if (!w.Write(b.UnkByte1)) return false;
    if (b.UnkByte1 == 0x02) {
        if (!w.Write(b.UnkVector0.x) || !w.Write(b.UnkVector0.y) || !w.Write(b.UnkVector0.z)) return false;
        if (!w.Write(b.UnkVector1.x) || !w.Write(b.UnkVector1.y) || !w.Write(b.UnkVector1.z)) return false;
        if (!w.Write(b.UnkUInt2)) return false;
    }
    if (!WriteSizedString(w, b.UnkString)) return false;
    if (!w.Write(b.RoomIndex)) return false;
    if (!w.Write(b.FloorIndex)) return false;

    uint32_t cnt = static_cast<uint32_t>(b.ConnectedBlockIndices.size());
    if (!w.Write(cnt)) return false;
    for (uint32_t idx : b.ConnectedBlockIndices) if (!w.Write(idx)) return false;

    cnt = static_cast<uint32_t>(b.VisibleBlockIndices.size());
    if (!w.Write(cnt)) return false;
    for (uint32_t idx : b.VisibleBlockIndices) if (!w.Write(idx)) return false;

    const uint32_t objCount = static_cast<uint32_t>(b.Objects.size());
    const uint32_t colObjCount = 0;
    if (!w.Write(objCount)) return false;
    if (!w.Write(colObjCount)) return false;
    for (const auto& o : b.Objects) {
        if (!WriteSizedString(w, o.Name)) return false;
        if (!WriteSizedString(w, o.Path)) return false;
        if (!w.Write(o.Position.x) || !w.Write(o.Position.y) || !w.Write(o.Position.z)) return false;
        if (!w.Write(o.Rotation.x) || !w.Write(o.Rotation.y) || !w.Write(o.Rotation.z)) return false;
        if (!w.Write(o.Scale.x) || !w.Write(o.Scale.y) || !w.Write(o.Scale.z)) return false;
        if (!w.Write(o.Flag)) return false;
        if (!w.Write(o.Int0)) return false;
        if (!w.Write(o.RadiusSqrt)) return false;
        if (o.Flag & 4) { if (!w.Write(o.WaterColor)) return false; }
    }

    const uint32_t lightCount = static_cast<uint32_t>(b.Lights.size());
    if (!w.Write(lightCount)) return false;
    for (const auto& l : b.Lights) {
        if (!WriteSizedString(w, l.Name)) return false;
        if (!w.Write(l.Position.x) || !w.Write(l.Position.y) || !w.Write(l.Position.z)) return false;
        for (int j = 0; j < 3; ++j) if (!w.Write(l.Color0[j])) return false;
        for (int j = 0; j < 3; ++j) if (!w.Write(l.Color1[j])) return false;
        for (int j = 0; j < 3; ++j) if (!w.Write(l.Color2[j])) return false;
        if (!w.Write(l.Float0) || !w.Write(l.Float1) || !w.Write(l.Float2)) return false;
    }
    return true;
}

bool BuildDofBuffer(const formats::DofDungeon& d, std::vector<uint8_t>& buf) {
    VectorWriter w(buf);
    std::string sig = d.Signature.empty() ? "JMXVDOF 1000" : d.Signature;
    if (sig.size() < 12) sig.resize(12, ' ');
    if (!w.WriteBytes(sig.data(), 12)) return false;

    const size_t offsetTablePos = buf.size();
    for (int i = 0; i < 8; ++i)
        if (!w.Write(static_cast<uint32_t>(0))) return false;

    if (!w.Write(d.Type)) return false;
    if (!WriteSizedString(w, d.Name)) return false;
    if (!w.Write(d.UnkUInt0)) return false;
    if (!w.Write(d.UnkUInt1)) return false;
    if (!w.Write(d.RegionID)) return false;

    uint32_t boundingBoxOffset = 0;
    if (d.BoundingBoxOffset > 0) {
        boundingBoxOffset = w.Size();
        for (int i = 0; i < 24; ++i) if (!w.Write(d.CollisionBox0[i])) return false;
        for (int i = 0; i < 24; ++i) if (!w.Write(d.CollisionBox1[i])) return false;
    }

    uint32_t blockOffset = 0;
    if (!d.Blocks.empty()) {
        blockOffset = w.Size();
        const uint32_t blockCount = static_cast<uint32_t>(d.Blocks.size());
        if (!w.Write(blockCount)) return false;
        for (const auto& block : d.Blocks)
            if (!WriteBlock(w, block)) return false;
    }

    uint32_t gridOffset = 0;
    if (d.GridOffset > 0 || !d.Voxels.empty() || d.GridWidth > 0) {
        gridOffset = w.Size();
        if (!w.Write(d.GridWidth)) return false;
        if (!w.Write(d.GridHeight)) return false;
        if (!w.Write(d.GridLength)) return false;
        const uint32_t voxelCount = static_cast<uint32_t>(d.Voxels.size());
        if (!w.Write(voxelCount)) return false;
        for (const auto& vx : d.Voxels) {
            if (!w.Write(vx.ID)) return false;
            const uint32_t bc = static_cast<uint32_t>(vx.BlockIndices.size());
            if (!w.Write(bc)) return false;
            for (uint32_t idx : vx.BlockIndices) if (!w.Write(idx)) return false;
        }
    }

    uint32_t linkOffset = 0;
    if (!d.Links.empty()) {
        linkOffset = w.Size();
        const uint32_t linkCount = static_cast<uint32_t>(d.Links.size());
        if (!w.Write(linkCount)) return false;
        for (const auto& link : d.Links) {
            const uint32_t bc = static_cast<uint32_t>(link.BlockIndices.size());
            if (!w.Write(bc)) return false;
            for (uint32_t idx : link.BlockIndices) if (!w.Write(idx)) return false;
        }
    }

    uint32_t groupOffset = 0;
    if (!d.Groups.empty()) {
        groupOffset = w.Size();
        const uint32_t groupCount = static_cast<uint32_t>(d.Groups.size());
        if (!w.Write(groupCount)) return false;
        for (const auto& g : d.Groups) {
            if (!WriteSizedString(w, g.Name)) return false;
            if (!w.Write(g.Flag)) return false;
            const uint32_t bc = static_cast<uint32_t>(g.BlockIndices.size());
            if (!w.Write(bc)) return false;
            for (uint32_t idx : g.BlockIndices) if (!w.Write(idx)) return false;
        }
    }

    uint32_t labelOffset = 0;
    if (d.LabelOffset != 0 || !d.Labels.Rooms.empty() || !d.Labels.Floors.empty()) {
        labelOffset = w.Size();
        const uint32_t roomCount = static_cast<uint32_t>(d.Labels.Rooms.size());
        if (!w.Write(roomCount)) return false;
        for (const auto& room : d.Labels.Rooms)
            if (!WriteSizedString(w, room)) return false;
        const uint32_t floorCount = static_cast<uint32_t>(d.Labels.Floors.size());
        if (!w.Write(floorCount)) return false;
        for (const auto& floor : d.Labels.Floors) {
            const uint16_t len = static_cast<uint16_t>(floor.size());
            if (!w.Write(len)) return false;
            if (len > 0 && !w.WriteBytes(floor.data(), len)) return false;
        }
    }

    const uint32_t offsets[8] = {
        blockOffset, linkOffset, gridOffset, groupOffset,
        labelOffset, d.Offset5, d.Offset6, boundingBoxOffset
    };
    std::memcpy(buf.data() + offsetTablePos, offsets, sizeof(offsets));
    return true;
}

} // namespace

ParseResult DofParser::Write(const std::wstring& path, const formats::DofDungeon& data) {
    std::vector<uint8_t> buf;
    if (!BuildDofBuffer(data, buf)) return ParseResult::Fail("Failed to serialize dungeon", path);

    BinaryWriter writer(path);
    if (!writer.IsOpen()) return ParseResult::Fail("Cannot create file", path);
    if (!writer.WriteBytes(buf.data(), buf.size())) return ParseResult::Fail("Write failed", path);
    return ParseResult::Success(path);
}

} // namespace sro
