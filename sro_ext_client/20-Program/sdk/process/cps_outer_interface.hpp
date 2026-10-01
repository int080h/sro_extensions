#pragma once

#include "sdk/process/cps_silkroad.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

class cprocess;
struct cprocess_msg;
class cps_version_check;
class cps_title;
class cps_character_select;

// CPSOuterInterface — extends CPSilkroad (+0xF0..+0x10B), vt @ 0x1032E34 (41 slots).
class cps_outer_interface : public cps_silkroad {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto get_gfx_child() -> void*;
  auto get_field_f4() -> int;
  auto get_timer_value() -> int;
  auto get_field_fc() -> int;
  auto get_msg_set() -> ext_client::msvc9::n_set<void*>&;
  auto get_res_ui_root() -> res_ui_root_map&;
  auto get_res_ui_root() const -> const res_ui_root_map&;
  auto get_login_phase() const -> int;
  auto get_net_state() const -> int;
  auto get_load_thread() const -> void*;

  auto set_gfx_child(void* val) -> void;
  auto set_field_f4(int val) -> void;
  auto set_timer_value(int val) -> void;
  auto set_field_fc(int val) -> void;
  auto set_net_state(int state) -> void;

  auto get_ui_child(int control_id, bool add_base_key = true) -> void*;
  auto find_child(int res_id) -> cgwnd*;
  auto get_res_map_key_for(const cgwnd* widget) -> int;
  auto walk_each(int max_depth, cgwnd::child_visitor_fn visit, void* ctx) -> void;
};
