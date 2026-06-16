#pragma once
#include "RegionCoord.h"
#include <cstdint>

namespace sro {

// Packed ushort: bit15=dungeon, bits8-14=rx (Z), bits0-7=ry (X)
struct RegionId {
    uint16_t value = 0;

    RegionId() = default;
    explicit RegionId(uint16_t v) : value(v) {}

    static RegionId FromCoord(const RegionCoord& c, bool dungeon = false) {
        uint16_t v = (static_cast<uint16_t>(c.rx) << 8) | c.ry;
        if (dungeon) v |= 0x8000;
        return RegionId(v);
    }

    static RegionId FromRxRy(int rx, int ry, bool dungeon = false) {
        return FromCoord(RegionCoord(rx, ry), dungeon);
    }

    RegionCoord ToCoord() const {
        return RegionCoord((value >> 8) & 0x7F, value & 0xFF);
    }

    int Rx() const { return (value >> 8) & 0xFF; }
    int Ry() const { return value & 0xFF; }
    bool IsDungeon() const { return (value & 0x8000) != 0; }
};

} // namespace sro
