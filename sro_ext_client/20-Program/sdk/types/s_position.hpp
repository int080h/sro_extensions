#pragma once

#include <cstdint>
#include <cstddef>

// s_position: 3D coordinates in Silkroad's regional space
struct s_position {
  std::uint16_t region_id;
  float x;
  float z;
  float y;

  auto region_x() const -> std::uint8_t {
    return static_cast<std::uint8_t>(region_id & 0xFF);
  }
  auto region_y() const -> std::uint8_t {
    return static_cast<std::uint8_t>(region_id >> 8);
  }
};

static_assert(sizeof(s_position) == 0x10);
static_assert(offsetof(s_position, x) == 0x04);
static_assert(offsetof(s_position, z) == 0x08);
static_assert(offsetof(s_position, y) == 0x0C);
