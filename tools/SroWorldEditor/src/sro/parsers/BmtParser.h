#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct BmtEntry {
    std::string Name;
    uint8_t HasTexture;
    std::string TextureName;
};

class BmtParser {
public:
    std::string Signature;
    std::vector<BmtEntry> Entries;

    bool Read(const std::wstring& path);
};
