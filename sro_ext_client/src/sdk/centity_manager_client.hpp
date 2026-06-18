#pragma once

#include "centity_manager.hpp"

#include <cstdint>

class cg_interface;

// CEntityManagerClient — in-game entity manager (extends CEntityManager + CIObject MI).
// Access: CGInterface → get_ui_child(0x19) → wrapper+0x7C4.

class centity_manager_client : public centity_manager {
public:
  static auto get() -> centity_manager_client*;
  static auto is_instance(const void* ptr) -> bool;
  static auto from_wrapper(void* wrapper) -> centity_manager_client*;

  auto lookup_entity_by_slot(std::uint32_t slot_id) const -> ci_charactor*;
};
