#include "MapMeshParser.h"
#include "io/BinaryReader.h"

namespace sro {

ParseResult MapMeshParser::Read(const std::wstring& path, formats::MapMesh& out) {
    BinaryReader reader(path);
    if (!reader.IsOpen()) return ParseResult::Fail("Cannot open file", path);

    if (!reader.ReadSignature(out.Signature)) return ParseResult::Fail("Failed to read signature", path);
    if (out.Signature.rfind("JMXVMAPM", 0) != 0) return ParseResult::Fail("Invalid JMXVMAPM signature", path);

    for (int i = 0; i < 36; ++i) {
        auto& block = out.Blocks[i];
        if (!reader.Read(block.Flag)) return ParseResult::Fail("Truncated block header", path);
        if (!reader.Read(block.EnvironmentID)) return ParseResult::Fail("Truncated block header", path);

        for (int j = 0; j < 289; ++j) {
            if (!reader.Read(block.Vertices[j].Height)) return ParseResult::Fail("Truncated vertices", path);
            if (!reader.Read(block.Vertices[j].TextureData)) return ParseResult::Fail("Truncated vertices", path);
            if (!reader.Read(block.Vertices[j].Brightness)) return ParseResult::Fail("Truncated vertices", path);
        }

        if (!reader.Read(block.WaterType)) return ParseResult::Fail("Truncated water", path);
        if (!reader.Read(block.WaterWaveType)) return ParseResult::Fail("Truncated water", path);
        if (!reader.Read(block.WaterHeight)) return ParseResult::Fail("Truncated water", path);

        for (int j = 0; j < 256; ++j) {
            if (!reader.Read(block.Tiles[j])) return ParseResult::Fail("Truncated tiles", path);
        }

        if (!reader.Read(block.HeightMax)) return ParseResult::Fail("Truncated block footer", path);
        if (!reader.Read(block.HeightMin)) return ParseResult::Fail("Truncated block footer", path);
        if (!reader.ReadBytes(block.Reserved, 20)) return ParseResult::Fail("Truncated reserved", path);
    }
    return ParseResult::Success(path);
}

ParseResult MapMeshParser::Write(const std::wstring& path, const formats::MapMesh& mesh) {
    BinaryWriter writer(path);
    if (!writer.IsOpen()) return ParseResult::Fail("Cannot create file", path);
    if (!writer.WriteSignature(mesh.Signature)) return ParseResult::Fail("Write signature failed", path);

    for (int i = 0; i < 36; ++i) {
        const auto& block = mesh.Blocks[i];
        if (!writer.Write(block.Flag)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(block.EnvironmentID)) return ParseResult::Fail("Write failed", path);

        for (int j = 0; j < 289; ++j) {
            if (!writer.Write(block.Vertices[j].Height)) return ParseResult::Fail("Write failed", path);
            if (!writer.Write(block.Vertices[j].TextureData)) return ParseResult::Fail("Write failed", path);
            if (!writer.Write(block.Vertices[j].Brightness)) return ParseResult::Fail("Write failed", path);
        }

        if (!writer.Write(block.WaterType)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(block.WaterWaveType)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(block.WaterHeight)) return ParseResult::Fail("Write failed", path);

        for (int j = 0; j < 256; ++j) {
            if (!writer.Write(block.Tiles[j])) return ParseResult::Fail("Write failed", path);
        }

        if (!writer.Write(block.HeightMax)) return ParseResult::Fail("Write failed", path);
        if (!writer.Write(block.HeightMin)) return ParseResult::Fail("Write failed", path);
        if (!writer.WriteBytes(block.Reserved, 20)) return ParseResult::Fail("Write failed", path);
    }
    return ParseResult::Success(path);
}

} // namespace sro
