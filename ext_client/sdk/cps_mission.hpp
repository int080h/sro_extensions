#pragma once

#include "cps_outer_interface.hpp"
#include "utils/offsets.hpp"

// CPSMission vtable (0x102F3B4, 41 methods) — same slot layout as CPSOuterInterface.
using cps_mission_vtable = cps_outer_interface_vtable;

// CPSMission: in-world / loading process (resinfo\psmission.txt). Size 0x170.
class cps_mission : public cps_outer_interface {
public:
  DECLARE_SDK_VTABLE(cps_mission_vtable, mission_vftable)

  static auto create() -> cps_mission*;
  static auto current() -> cps_mission*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_mission*;

private:
  PAD(ext_client::offsets::cps_mission::size - ext_client::offsets::cps_outer_interface::derived_region_begin);

  static inline auto check_layout() -> void {
    static_assert(sizeof(cps_mission) == ext_client::offsets::cps_mission::size, "cps_mission size mismatch");
  }
};
