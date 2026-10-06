#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSTitle — Login / Server / Channel screen process (resinfo\pstitle.txt)
// Native VTable: 0x010344F4 (41 slots) | Class Size: 0x218 | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_title : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x010344F4;
  static constexpr std::size_t   k_class_size  = 0x0218;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_title*;
  static auto resolve_live() -> cps_title*;
  static auto is_instance(const void* ptr) -> bool;
  static auto is_live(const void* ptr) -> bool;
  static auto create() -> cps_title*;

  // 2. Login & Server Selection State
  auto is_captcha_active() const -> bool;
  auto get_autologin_dialog() const -> void*;
  static auto get_channel_index() -> int;

  // 3. Native Actions
  auto trigger_login() -> int;
};
