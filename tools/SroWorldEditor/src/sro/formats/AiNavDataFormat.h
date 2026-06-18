#pragma once
#include <cstdint>
#include <vector>

namespace sro::formats {

struct AiNavRefCell {
    int16_t RefEdgeIndex0 = -1;
    int16_t RefEdgeIndex1 = -1;
};

struct AiNavBlockLink {
    uint32_t ID = 0;
    uint16_t CellID = 0;
    uint16_t LinkedObjID = 0;
    uint16_t LinkedObjRefEdgeIndex = 0;
};

struct AiNavRefBlock {
    uint32_t Index = 0;
    uint32_t CellCount = 0;
    uint32_t EdgeCount = 0;
    std::vector<AiNavRefCell> CellLookupTable;
    std::vector<AiNavBlockLink> Links;
};

struct AiNavRefDungeon {
    uint16_t RegionID = 0;
    uint32_t BlockCount = 0;
    std::vector<AiNavRefBlock> Blocks;
    std::vector<int16_t> BlockLookupTable;
    uint32_t Int0 = 0;
};

struct AiNavSimpleBlock {
    uint32_t EdgeCount = 0;
    std::vector<float> EdgeCenterX;
    std::vector<float> EdgeCenterY;
    std::vector<float> EdgeCenterZ;
};

struct AiNavSimpleDungeonData {
    uint16_t RegionID = 0;
    uint32_t BlockCount = 0;
    std::vector<AiNavSimpleBlock> Blocks;
    uint32_t Int1 = 0;
};

struct AiNavData {
    uint8_t Version = 1;
    uint32_t SimpleDungeonDataOffset = 0;
    AiNavRefDungeon RefDungeon;
    AiNavSimpleDungeonData SimpleDungeon;
};

} // namespace sro::formats
