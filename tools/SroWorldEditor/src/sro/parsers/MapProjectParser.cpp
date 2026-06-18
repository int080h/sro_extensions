#include "MapProjectParser.h"
#include "io/BinaryReader.h"
#include <cstring>

namespace sro {

ParseResult MapProjectParser::Read(const std::wstring& path, formats::MapProject& out) {
    BinaryReader reader(path);
    if (!reader.IsOpen()) return ParseResult::Fail("Cannot open file", path);

    if (!reader.ReadSignature(out.Signature)) return ParseResult::Fail("Failed to read signature", path);
    if (out.Signature.rfind("JMXVMFO", 0) != 0) return ParseResult::Fail("Invalid JMXVMFO signature", path);

    if (!reader.Read(out.MapWidth)) return ParseResult::Fail("Truncated header", path);
    if (!reader.Read(out.MapHeight)) return ParseResult::Fail("Truncated header", path);

    reader.Seek(reader.Tell() + std::streampos(8));

    uint8_t bitData[8192];
    if (!reader.ReadBytes(bitData, 8192)) return ParseResult::Fail("Truncated region bitmap", path);

    out.ActiveRegions.clear();
    // JMXVMFO RegionData: 256x256 bit array, rx-major (same order as world map grid).
    for (int ry = 0; ry < 256; ++ry) {
        for (int rx = 0; rx < 256; ++rx) {
            int bitIdx = rx * 256 + ry;
            int byteIdx = bitIdx / 8;
            int bitOffset = bitIdx % 8;
            if (byteIdx < 8192 && (bitData[byteIdx] & (1 << bitOffset)) != 0) {
                out.ActiveRegions.insert({rx, ry});
            }
        }
    }
    return ParseResult::Success(path);
}

ParseResult MapProjectParser::Write(const std::wstring& path, const formats::MapProject& in) {
    BinaryWriter writer(path);
    if (!writer.IsOpen()) return ParseResult::Fail("Cannot open file", path);

    std::string sig = in.Signature.empty() ? "JMXVMFO 1000" : in.Signature;
    if (sig.size() < 12) sig.resize(12, '\0');
    if (!writer.WriteSignature(sig)) return ParseResult::Fail("Write signature failed", path);

    int16_t w = in.MapWidth ? in.MapWidth : 256;
    int16_t h = in.MapHeight ? in.MapHeight : 128;
    if (!writer.Write(w)) return ParseResult::Fail("Write failed", path);
    if (!writer.Write(h)) return ParseResult::Fail("Write failed", path);

    uint8_t zeros[8] = {};
    if (!writer.WriteBytes(zeros, 8)) return ParseResult::Fail("Write failed", path);

    uint8_t bitData[8192] = {};
    for (const auto& [rx, ry] : in.ActiveRegions) {
        int bitIdx = rx * 256 + ry;
        int byteIdx = bitIdx / 8;
        int bitOffset = bitIdx % 8;
        if (byteIdx < 8192) bitData[byteIdx] |= static_cast<uint8_t>(1 << bitOffset);
    }
    if (!writer.WriteBytes(bitData, 8192)) return ParseResult::Fail("Write bitmap failed", path);
    return ParseResult::Success(path);
}

} // namespace sro
