#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSMission — In-world / loading screen process (resinfo\psmission.txt)
// Native VTable: 0x0102F3B4 (41 slots) | Class Size: 0x170 | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_mission : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x0102F3B4;
  static constexpr std::size_t   k_class_size  = 0x0170;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_mission*;
  static auto create() -> cps_mission*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_mission*;
};
