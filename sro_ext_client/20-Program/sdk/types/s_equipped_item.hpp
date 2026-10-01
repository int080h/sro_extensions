#pragma once

#include <cstdint>

struct s_equipped_item {
  std::uint32_t model_id;
  std::uint8_t opt_level;
  std::uint8_t pad[7];
};
