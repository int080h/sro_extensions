#pragma once

#include "sdk/game/centity_manager.hpp"

#include <cstdint>

class cg_interface;

// ---------------------------------------------------------------------------
// CEntityManagerClient — In-game client entity manager (local player & spawns)
// RTTI: "CEntityManagerClient" | Extends CEntityManager + CIObject MI
// Resolved via CGInterface::GetUIChild(0x19) wrapper @ 0x008831F0
// ---------------------------------------------------------------------------
class centity_manager_client : public centity_manager {
public:
  // 1. Singleton & Type Inspection
  static auto get() -> centity_manager_client*;
  static auto is_instance(const void* ptr) -> bool;
  static auto cast(void* ptr) -> centity_manager_client*;
  static auto cast(const void* ptr) -> const centity_manager_client*;
  static auto from_wrapper(void* wrapper) -> centity_manager_client*;

  // 2. Entity Queries
  auto lookup_entity_by_slot(std::uint32_t slot_id) const -> ci_charactor*;
};
