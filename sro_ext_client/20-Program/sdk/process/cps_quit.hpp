#pragma once

#include "sdk/process/cps_silkroad.hpp"

#include <cstdint>

// CPSQuit: quit confirmation dialog (resinfo\psquit.txt).
// vt @ 0x102F47C (41 slots). Size 0xE4.
class cps_quit : public cps_silkroad {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto is_quit_flag() const -> bool;
  auto set_quit_flag(bool val) -> void;

  static auto create() -> cps_quit*;
  static auto current() -> cps_quit*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_quit*;
};
