#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstdint>

// CPSTitle: login / server / channel screen (resinfo\pstitle.txt).
// vt @ 0x10344F4 (41 slots). Size 0x218.
class cps_title : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto is_captcha_active() const -> bool;
  auto get_autologin_dialog() const -> void*;
  auto trigger_login() -> int;

  static auto create() -> cps_title*;
  static auto current() -> cps_title*;
  static auto is_instance(const void* ptr) -> bool;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_title*;
  static auto get_channel_index() -> int;
};
