#pragma once

#include "sdk/process/cprocess.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

using res_ui_root_map = ext_client::msvc9::n_map<int, void*>;

// ---------------------------------------------------------------------------
// CPSSilkroad — Main In-Game Game State process base class
// Native VTable: 0x0102EFFC (41 slots) | Extends CProcess (+0xB0..+0xEF)
// ---------------------------------------------------------------------------
class cps_silkroad : public cprocess {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x0102EFFC;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. UI Root Map & Resources
  auto get_res_ui_root() -> res_ui_root_map&;
  auto get_res_ui_root() const -> const res_ui_root_map&;
  auto get_res_loader() -> void*;
  auto set_res_loader(void* val) -> void;

  // 2. Login Phase & Mode
  auto get_login_phase() const -> int;
  auto get_login_mode() const -> int;
  auto set_login_phase(int val) -> void;
  auto set_login_mode(int val) -> void;
};
