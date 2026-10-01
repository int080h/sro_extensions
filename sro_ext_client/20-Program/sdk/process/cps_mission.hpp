#pragma once

#include "sdk/process/cps_outer_interface.hpp"

// CPSMission: in-world / loading process (resinfo\psmission.txt).
// vt @ 0x102F3B4 (41 slots). Size 0x170.
class cps_mission : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  static auto create() -> cps_mission*;
  static auto current() -> cps_mission*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_mission*;
};
