#pragma once

#include <cstdint>

// s_action_slot: slot structure for emotes and actions
struct s_action_slot {
  std::uint8_t action_type;
  std::uint8_t pad[7];
  std::uint32_t action_id;
  std::uint8_t pad2[8];
};
