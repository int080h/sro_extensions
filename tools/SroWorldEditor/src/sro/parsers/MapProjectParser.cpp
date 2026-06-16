#include "MapProjectParser.h"
#include "io/BinaryReader.h"

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

} // namespace sro
