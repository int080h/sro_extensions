#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstdint>

// CPSQuickStart: quick start / auto-login screen (resinfo\psquickstart.txt).
// vt @ 0x1032F04 (41 slots). Size 0x10C.
class cps_quick_start : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  static auto create() -> cps_quick_start*;
  static auto current() -> cps_quick_start*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_quick_start*;
};
