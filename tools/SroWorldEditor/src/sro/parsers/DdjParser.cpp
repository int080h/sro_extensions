#include "DdjParser.h"
#include <fstream>

std::vector<uint8_t> DdjParser::LoadToDdsBytes(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary | std::ios::ate);
    if (!fs.is_open()) return {};

    size_t size = fs.tellg();
    if (size < 24) return {};

    fs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    fs.read(reinterpret_cast<char*>(buffer.data()), size);

    // DDS magic is always at offset 20 in Silkroad DDJ files
    if (buffer[20] == 0x44 && buffer[21] == 0x44 && buffer[22] == 0x53 && buffer[23] == 0x20) {
        return std::vector<uint8_t>(buffer.begin() + 20, buffer.end());
    }
    return {};
}
