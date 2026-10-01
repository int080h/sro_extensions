#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstdint>

// CPSRestart: restart / reconnect screen (resinfo\psrestart.txt).
// vt @ 0x1033214 (41 slots). Size 0x10C.
class cps_restart : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  static auto create() -> cps_restart*;
  static auto current() -> cps_restart*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_restart*;
};
