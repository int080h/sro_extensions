#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstdint>

// CPSLogo: logo splash screen (resinfo\pslogo.txt).
// vt @ 0x1032A9C (41 slots). Size 0x118.
class cps_logo : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  static auto create() -> cps_logo*;
  static auto current() -> cps_logo*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_logo*;
};
