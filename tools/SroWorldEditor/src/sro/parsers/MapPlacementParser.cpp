#include "MapPlacementParser.h"
#include "io/BinaryReader.h"
#include "io/PathHelpers.h"

namespace sro {

bool MapPlacementParser::ReadObject(BinaryReader& reader, formats::MapObject& obj, bool hasRegionId) {
    if (!reader.Read(obj.ObjID)) return false;
    if (!reader.Read(obj.PosX)) return false;
    if (!reader.Read(obj.PosY)) return false;
    if (!reader.Read(obj.PosZ)) return false;
    if (!reader.Read(obj.IsStatic)) return false;
    if (!reader.Read(obj.Yaw)) return false;
    if (!reader.Read(obj.UID)) return false;
    if (!reader.Read(obj.Short0)) return false;
    if (!reader.Read(obj.IsBig)) return false;
    if (!reader.Read(obj.IsStruct)) return false;
    if (hasRegionId) {
        if (!reader.Read(obj.RegionID)) return false;
    } else {
        obj.RegionID = 0;
    }
    return true;
}

ParseResult MapPlacementParser::ReadLegacy(BinaryReader& reader, formats::MapPlacements& out, const std::wstring& path) {
    out.Format = formats::PlacementFormat::LegacyO;
    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            for (int lod = 1; lod < 4; ++lod) {
                out.Blocks[z][x][lod].clear();
            }
            uint16_t count = 0;
            if (!reader.Read(count)) return ParseResult::Fail("Truncated block count", path);
            if (count > 2000) return ParseResult::Fail("Object count too large", path);
            out.Blocks[z][x][0].resize(count);
            for (uint16_t i = 0; i < count; ++i) {
                if (!ReadObject(reader, out.Blocks[z][x][0][i], false)) {
                    return ParseResult::Fail("Truncated object data", path);
                }
            }
        }
    }
    return ParseResult::Success(path);
}

ParseResult MapPlacementParser::ReadLod(BinaryReader& reader, formats::MapPlacements& out, const std::wstring& path) {
    out.Format = formats::PlacementFormat::LodO2;
    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            for (int lod = 0; lod < 4; ++lod) {
                uint16_t count = 0;
                if (!reader.Read(count)) return ParseResult::Fail("Truncated LOD count", path);
                if (count > 2000) return ParseResult::Fail("Object count too large", path);
                out.Blocks[z][x][lod].resize(count);
                for (uint16_t i = 0; i < count; ++i) {
                    if (!ReadObject(reader, out.Blocks[z][x][lod][i], true)) {
                        return ParseResult::Fail("Truncated object data", path);
                    }
                }
            }
        }
    }
    return ParseResult::Success(path);
}

ParseResult MapPlacementParser::Read(const std::wstring& path, formats::MapPlacements& out) {
    BinaryReader reader(path);
    if (!reader.IsOpen()) return ParseResult::Fail("Cannot open file", path);

    if (!reader.ReadSignature(out.Signature)) return ParseResult::Fail("Failed to read signature", path);
    if (out.Signature.rfind("JMXVMAPO", 0) != 0) return ParseResult::Fail("Invalid JMXVMAPO signature", path);

    if (IsO2Path(path)) {
        return ReadLod(reader, out, path);
    }
    return ReadLegacy(reader, out, path);
}

ParseResult MapPlacementParser::Write(const std::wstring& path, const formats::MapPlacements& placements) {
    BinaryWriter writer(path);
    if (!writer.IsOpen()) return ParseResult::Fail("Cannot create file", path);
    if (!writer.WriteSignature(placements.Signature)) return ParseResult::Fail("Write signature failed", path);

    const bool isO2 = placements.Format == formats::PlacementFormat::LodO2 || IsO2Path(path);
    const int lodCount = isO2 ? 4 : 1;
    const bool hasRegionId = isO2;

    for (int z = 0; z < 6; ++z) {
        for (int x = 0; x < 6; ++x) {
            for (int lod = 0; lod < lodCount; ++lod) {
                const auto& list = placements.Blocks[z][x][lod];
                uint16_t count = static_cast<uint16_t>(list.size());
                if (!writer.Write(count)) return ParseResult::Fail("Write failed", path);
                for (const auto& obj : list) {
                    if (!writer.Write(obj.ObjID)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.PosX)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.PosY)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.PosZ)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.IsStatic)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.Yaw)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.UID)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.Short0)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.IsBig)) return ParseResult::Fail("Write failed", path);
                    if (!writer.Write(obj.IsStruct)) return ParseResult::Fail("Write failed", path);
                    if (hasRegionId) {
                        if (!writer.Write(obj.RegionID)) return ParseResult::Fail("Write failed", path);
                    }
                }
            }
        }
    }
    return ParseResult::Success(path);
}

} // namespace sro
