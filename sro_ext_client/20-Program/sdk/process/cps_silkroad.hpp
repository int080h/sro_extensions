#pragma once

#include "sdk/process/cprocess.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

using res_ui_root_map = ext_client::msvc9::n_map<int, void*>;

// CPSilkroad — extends CProcess (+0xB0..+0xEF), vt @ 0x102EFFC (41 slots).
class cps_silkroad : public cprocess {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto get_res_ui_root() -> res_ui_root_map&;
  auto get_res_ui_root() const -> const res_ui_root_map&;
  auto get_res_loader() -> void*;
  auto get_login_phase() -> int;
  auto get_login_mode() -> int;

  auto set_res_loader(void* val) -> void;
  auto set_login_phase(int val) -> void;
  auto set_login_mode(int val) -> void;
};
