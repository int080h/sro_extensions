#pragma once
#include <string>
#include <set>
#include <utility>
#include <cstdint>

namespace sro::formats {

struct MapProject {
    std::string Signature;
    int16_t MapWidth = 0;
    int16_t MapHeight = 0;
    std::set<std::pair<int, int>> ActiveRegions;
};

} // namespace sro::formats
