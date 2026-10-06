#pragma once

#include "sdk/process/cps_silkroad.hpp"
#include "sdk/net/cmsg_stream_buffer.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

class cprocess;
struct cprocess_msg;
class cps_version_check;
class cps_title;
class cps_character_select;

// ---------------------------------------------------------------------------
// CPSOuterInterface — Base state for outer lobby screens (Title, VersionCheck, CharSelect)
// Native VTable: 0x01032E34 (41 slots) | Extends CPSSilkroad (+0xF0..+0x10B)
// ---------------------------------------------------------------------------
class cps_outer_interface : public cps_silkroad {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01032E34;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. UI Map & Widget Lookups
  auto get_ui_child(int control_id, bool add_base_key = true) -> cgwnd*;
  auto find_child(int res_id) -> cgwnd*;
  auto get_res_ui_root() -> res_ui_root_map&;
  auto get_res_ui_root() const -> const res_ui_root_map&;
  auto get_res_map_key_for(const cgwnd* widget) -> int;

  // 2. Process State & Timers
  auto get_login_phase() const -> int;
  auto get_net_state() const -> int;
  auto set_net_state(int state) -> void;
  auto get_timer_value() const -> int;
  auto set_timer_value(int val) -> void;
  auto get_load_thread() const -> void*;

  // 3. Child Traversal
  auto walk_each(int max_depth, cgwnd::child_visitor_fn visit, void* ctx) -> void;
};
