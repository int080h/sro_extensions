#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSRestart — Restart / Reconnect screen process (resinfo\psrestart.txt)
// Native VTable: 0x01033214 (41 slots) | Class Size: 0x10C | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_restart : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01033214;
  static constexpr std::size_t   k_class_size  = 0x010C;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_restart*;
  static auto create() -> cps_restart*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_restart*;
};
