#pragma once

#include "sdk/cicharactor.hpp"

#include <cstdint>

namespace ext_client::combat_target {

  auto selected_slot_id() -> std::uint32_t;
  auto get_by_slot(std::uint32_t slot_id) -> ci_charactor*;
  auto get_selected() -> ci_charactor*;

} // namespace ext_client::combat_target
