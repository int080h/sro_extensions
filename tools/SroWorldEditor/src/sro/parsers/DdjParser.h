#pragma once
#include <string>
#include <vector>
#include <cstdint>

class DdjParser {
public:
    static std::vector<uint8_t> LoadToDdsBytes(const std::wstring& path);
};
