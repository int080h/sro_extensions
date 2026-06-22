#pragma once

#include "cps_outer_interface.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

// CPSTitle vtable (0x10344F4, 41 methods) — same slot layout as CPSOuterInterface.
using cps_title_vtable = cps_outer_interface_vtable;

// CPSTitle: login / server / channel screen (resinfo\pstitle.txt). Size 0x218.
class cps_title : public cps_outer_interface {
public:
  DECLARE_SDK_VTABLE(cps_title_vtable, title_vftable)

  static auto create() -> cps_title*;
  static auto current() -> cps_title*;
  static auto is_instance(const void* ptr) -> bool;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_title*;
  static auto channel_index() -> int;

  auto captcha_active() const -> bool;
  auto autologin_dialog() const -> void*;

  auto trigger_login() -> int;

private:
  union {
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cps_title::size - sizeof(cps_outer_interface)], "pad_end");
  };

  static inline auto check_layout() -> void {
    
  }
};
