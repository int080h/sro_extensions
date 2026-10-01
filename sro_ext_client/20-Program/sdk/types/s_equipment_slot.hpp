#pragma once

#include <cstdint>

// s_equipment_slot: slot structure for items/equipment
struct s_equipment_slot {
  std::uint32_t item_id;
  std::uint8_t pad[16];
};
