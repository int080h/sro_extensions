#pragma once

#include "sdk/process/cps_silkroad.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSQuit — Quit confirmation dialog process (resinfo\psquit.txt)
// Native VTable: 0x0102F47C (41 slots) | Class Size: 0x0E4 | Extends CPSSilkroad
// ---------------------------------------------------------------------------
class cps_quit : public cps_silkroad {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x0102F47C;
  static constexpr std::size_t   k_class_size  = 0x00E4;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_quit*;
  static auto create() -> cps_quit*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_quit*;

  // 2. Quit State
  auto is_quit_flag() const -> bool;
  auto set_quit_flag(bool val) -> void;
};
