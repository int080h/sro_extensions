#include "AiNavDataParser.h"
#include "io/BinaryReader.h"
#include <fstream>
#include <cstring>

namespace sro {

namespace {

class BufferReader {
public:
    BufferReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    bool ReadBytes(void* dst, size_t count) {
        if (m_pos + count > m_size) return false;
        std::memcpy(dst, m_data + m_pos, count);
        m_pos += count;
        return true;
    }

    template<typename T>
    bool Read(T& out) { return ReadBytes(&out, sizeof(T)); }

    void Seek(size_t pos) { m_pos = pos; }

private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_pos = 0;
};

bool ReadRefDungeon(BinaryReader& reader, formats::AiNavRefDungeon& out) {
    if (!reader.Read(out.RegionID)) return false;
    if (!reader.Read(out.BlockCount)) return false;

    out.Blocks.resize(out.BlockCount);
    for (uint32_t i = 0; i < out.BlockCount; ++i) {
        auto& block = out.Blocks[i];
        if (!reader.Read(block.Index)) return false;
        if (!reader.Read(block.CellCount)) return false;
        if (!reader.Read(block.EdgeCount)) return false;

        const size_t lutSize = static_cast<size_t>(block.CellCount) * block.CellCount;
        block.CellLookupTable.resize(lutSize);
        for (size_t j = 0; j < lutSize; ++j) {
            if (!reader.Read(block.CellLookupTable[j].RefEdgeIndex0)) return false;
            if (!reader.Read(block.CellLookupTable[j].RefEdgeIndex1)) return false;
        }

        uint32_t linkCount = 0;
        if (!reader.Read(linkCount)) return false;
        block.Links.resize(linkCount);
        for (uint32_t j = 0; j < linkCount; ++j) {
            auto& link = block.Links[j];
            if (!reader.Read(link.ID)) return false;
            if (!reader.Read(link.CellID)) return false;
            if (!reader.Read(link.LinkedObjID)) return false;
            if (!reader.Read(link.LinkedObjRefEdgeIndex)) return false;
        }
    }

    const size_t bltSize = static_cast<size_t>(out.BlockCount) * out.BlockCount;
    out.BlockLookupTable.resize(bltSize);
    for (size_t i = 0; i < bltSize; ++i) {
        if (!reader.Read(out.BlockLookupTable[i])) return false;
    }
    return reader.Read(out.Int0);
}

bool ReadRefDungeon(BufferReader& reader, formats::AiNavRefDungeon& out) {
    if (!reader.Read(out.RegionID)) return false;
    if (!reader.Read(out.BlockCount)) return false;

    out.Blocks.resize(out.BlockCount);
    for (uint32_t i = 0; i < out.BlockCount; ++i) {
        auto& block = out.Blocks[i];
        if (!reader.Read(block.Index)) return false;
        if (!reader.Read(block.CellCount)) return false;
        if (!reader.Read(block.EdgeCount)) return false;

        const size_t lutSize = static_cast<size_t>(block.CellCount) * block.CellCount;
        block.CellLookupTable.resize(lutSize);
        for (size_t j = 0; j < lutSize; ++j) {
            if (!reader.Read(block.CellLookupTable[j].RefEdgeIndex0)) return false;
            if (!reader.Read(block.CellLookupTable[j].RefEdgeIndex1)) return false;
        }

        uint32_t linkCount = 0;
        if (!reader.Read(linkCount)) return false;
        block.Links.resize(linkCount);
        for (uint32_t j = 0; j < linkCount; ++j) {
            auto& link = block.Links[j];
            if (!reader.Read(link.ID)) return false;
            if (!reader.Read(link.CellID)) return false;
            if (!reader.Read(link.LinkedObjID)) return false;
            if (!reader.Read(link.LinkedObjRefEdgeIndex)) return false;
        }
    }

    const size_t bltSize = static_cast<size_t>(out.BlockCount) * out.BlockCount;
    out.BlockLookupTable.resize(bltSize);
    for (size_t i = 0; i < bltSize; ++i) {
        if (!reader.Read(out.BlockLookupTable[i])) return false;
    }
    return reader.Read(out.Int0);
}

bool ReadSimpleDungeon(BinaryReader& reader, formats::AiNavSimpleDungeonData& out) {
    if (!reader.Read(out.RegionID)) return false;
    if (!reader.Read(out.BlockCount)) return false;

    out.Blocks.resize(out.BlockCount);
    for (uint32_t i = 0; i < out.BlockCount; ++i) {
        auto& block = out.Blocks[i];
        if (!reader.Read(block.EdgeCount)) return false;
        block.EdgeCenterX.resize(block.EdgeCount);
        block.EdgeCenterY.resize(block.EdgeCount);
        block.EdgeCenterZ.resize(block.EdgeCount);
        for (uint32_t j = 0; j < block.EdgeCount; ++j) {
            if (!reader.Read(block.EdgeCenterX[j])) return false;
            if (!reader.Read(block.EdgeCenterY[j])) return false;
            if (!reader.Read(block.EdgeCenterZ[j])) return false;
        }
    }
    return reader.Read(out.Int1);
}

bool ReadSimpleDungeon(BufferReader& reader, formats::AiNavSimpleDungeonData& out) {
    if (!reader.Read(out.RegionID)) return false;
    if (!reader.Read(out.BlockCount)) return false;

    out.Blocks.resize(out.BlockCount);
    for (uint32_t i = 0; i < out.BlockCount; ++i) {
        auto& block = out.Blocks[i];
        if (!reader.Read(block.EdgeCount)) return false;
        block.EdgeCenterX.resize(block.EdgeCount);
        block.EdgeCenterY.resize(block.EdgeCount);
        block.EdgeCenterZ.resize(block.EdgeCount);
        for (uint32_t j = 0; j < block.EdgeCount; ++j) {
            if (!reader.Read(block.EdgeCenterX[j])) return false;
            if (!reader.Read(block.EdgeCenterY[j])) return false;
            if (!reader.Read(block.EdgeCenterZ[j])) return false;
        }
    }
    return reader.Read(out.Int1);
}

size_t ComputeRefDungeonSize(const formats::AiNavRefDungeon& d) {
    size_t size = 6;
    for (const auto& block : d.Blocks) {
        size += 12;
        size += static_cast<size_t>(block.CellCount) * block.CellCount * 4;
        size += 4;
        size += block.Links.size() * 10;
    }
    size += static_cast<size_t>(d.BlockCount) * d.BlockCount * 2;
    size += 4;
    return size;
}

} // namespace

ParseResult AiNavDataParser::ReadFromBuffer(const uint8_t* data, size_t size, formats::AiNavData& out) {
    if (!data || size < 5) return ParseResult::Fail("Buffer too small", L"");

    BufferReader reader(data, size);
    out = {};
    if (!reader.Read(out.Version)) return ParseResult::Fail("Truncated header", L"");
    if (!reader.Read(out.SimpleDungeonDataOffset)) return ParseResult::Fail("Truncated header", L"");
    if (!ReadRefDungeon(reader, out.RefDungeon)) return ParseResult::Fail("Truncated ref dungeon", L"");

    reader.Seek(out.SimpleDungeonDataOffset);
    if (!ReadSimpleDungeon(reader, out.SimpleDungeon))
        return ParseResult::Fail("Truncated simple dungeon data", L"");

    return ParseResult::Success(L"");
}

ParseResult AiNavDataParser::Read(const std::wstring& path, formats::AiNavData& out) {
    BinaryReader reader(path);
    if (!reader.IsOpen()) return ParseResult::Fail("Cannot open file", path);

    out = {};
    if (!reader.Read(out.Version)) return ParseResult::Fail("Truncated header", path);
    if (!reader.Read(out.SimpleDungeonDataOffset)) return ParseResult::Fail("Truncated header", path);
    if (!ReadRefDungeon(reader, out.RefDungeon)) return ParseResult::Fail("Truncated ref dungeon", path);

    reader.Seek(static_cast<std::streampos>(out.SimpleDungeonDataOffset));
    if (!ReadSimpleDungeon(reader, out.SimpleDungeon))
        return ParseResult::Fail("Truncated simple dungeon data", path);

    return ParseResult::Success(path);
}

std::vector<uint8_t> AiNavDataParser::WriteToBuffer(const formats::AiNavData& data) {
    formats::AiNavData copy = data;
    copy.SimpleDungeonDataOffset = static_cast<uint32_t>(5 + ComputeRefDungeonSize(copy.RefDungeon));

    std::vector<uint8_t> buf;
    buf.reserve(copy.SimpleDungeonDataOffset + 256);

    auto append = [&](const void* p, size_t n) {
        const auto* b = static_cast<const uint8_t*>(p);
        buf.insert(buf.end(), b, b + n);
    };
    auto writePod = [&](const auto& v) { append(&v, sizeof(v)); };

    writePod(copy.Version);
    writePod(copy.SimpleDungeonDataOffset);
    writePod(copy.RefDungeon.RegionID);
    writePod(copy.RefDungeon.BlockCount);
    for (const auto& block : copy.RefDungeon.Blocks) {
        writePod(block.Index);
        writePod(block.CellCount);
        writePod(block.EdgeCount);
        for (const auto& cell : block.CellLookupTable) {
            writePod(cell.RefEdgeIndex0);
            writePod(cell.RefEdgeIndex1);
        }
        const uint32_t linkCount = static_cast<uint32_t>(block.Links.size());
        writePod(linkCount);
        for (const auto& link : block.Links) {
            writePod(link.ID);
            writePod(link.CellID);
            writePod(link.LinkedObjID);
            writePod(link.LinkedObjRefEdgeIndex);
        }
    }
    for (int16_t v : copy.RefDungeon.BlockLookupTable) writePod(v);
    writePod(copy.RefDungeon.Int0);

    writePod(copy.SimpleDungeon.RegionID);
    writePod(copy.SimpleDungeon.BlockCount);
    for (const auto& block : copy.SimpleDungeon.Blocks) {
        writePod(block.EdgeCount);
        for (uint32_t j = 0; j < block.EdgeCount; ++j) {
            writePod(block.EdgeCenterX[j]);
            writePod(block.EdgeCenterY[j]);
            writePod(block.EdgeCenterZ[j]);
        }
    }
    writePod(copy.SimpleDungeon.Int1);
    return buf;
}

ParseResult AiNavDataParser::Write(const std::wstring& path, const formats::AiNavData& data) {
    auto buf = WriteToBuffer(data);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return ParseResult::Fail("Cannot open file for write", path);
    out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    if (!out) return ParseResult::Fail("Write failed", path);
    return ParseResult::Success(path);
}

} // namespace sro
