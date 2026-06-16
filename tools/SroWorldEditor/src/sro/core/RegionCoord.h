#pragma once
#include <cstdint>
#include <functional>

namespace sro {

struct RegionCoord {
    uint8_t rx = 0;
    uint8_t ry = 0;

    RegionCoord() = default;
    RegionCoord(int rx_, int ry_) : rx(static_cast<uint8_t>(rx_)), ry(static_cast<uint8_t>(ry_)) {}

    bool operator==(const RegionCoord& o) const { return rx == o.rx && ry == o.ry; }
    bool operator!=(const RegionCoord& o) const { return !(*this == o); }
    bool operator<(const RegionCoord& o) const {
        if (rx != o.rx) return rx < o.rx;
        return ry < o.ry;
    }
};

} // namespace sro

namespace std {
template<>
struct hash<sro::RegionCoord> {
    size_t operator()(const sro::RegionCoord& c) const noexcept {
        return (static_cast<size_t>(c.rx) << 8) | c.ry;
    }
};
} // namespace std
