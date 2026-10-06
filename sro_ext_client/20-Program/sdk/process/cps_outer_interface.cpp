#include "pch.hpp"
#include "sdk/process/cps_outer_interface.hpp"

#include "sdk/ui/cgwnd.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

auto cps_outer_interface::get_timer_value() const -> int {
  return ext_client::off::field_at<int>(this, 0x0F8);
}

auto cps_outer_interface::set_timer_value(int val) -> void {
  ext_client::off::field_at<int>(this, 0x0F8) = val;
}

auto cps_outer_interface::get_res_ui_root() -> res_ui_root_map& {
  return ext_client::off::field_at<res_ui_root_map>(this, 0x0B0);
}

auto cps_outer_interface::get_res_ui_root() const -> const res_ui_root_map& {
  return ext_client::off::field_at<res_ui_root_map>(this, 0x0B0);
}

auto cps_outer_interface::get_login_phase() const -> int {
  return ext_client::off::field_at<int>(this, 0x0E4);
}

auto cps_outer_interface::get_net_state() const -> int {
  return ext_client::off::field_at<int>(this, 0x084);
}

auto cps_outer_interface::set_net_state(int state) -> void {
  ext_client::off::field_at<int>(this, 0x084) = state;
}

auto cps_outer_interface::get_load_thread() const -> void* {
  return ext_client::off::field_at<void*>(this, 0x0A0);
}

auto cps_outer_interface::walk_each(int max_depth, cgwnd::child_visitor_fn visit, void* ctx) -> void {
  cgwnd::walk_each(max_depth, visit, ctx);
}

auto cps_outer_interface::find_child(int res_id) -> cgwnd* {
  if (res_id < 0) {
    return nullptr;
  }

  if (auto* widget = get_ui_child(res_id, true)) {
    if (widget && widget->is_live()) {
      return widget;
    }
  }

  if (auto* widget = cgwnd::get_child_by_unique_id(this, res_id)) {
    if (widget && widget->is_live()) {
      return widget;
    }
  }

  return nullptr;
}

auto cps_outer_interface::get_res_map_key_for(const cgwnd* widget) -> int {
  if (!widget) {
    return -1;
  }

  const auto& map = get_res_ui_root();
  int found = -1;
  map.for_each([&](int key, void* value) {
    if (value == widget) {
      found = key;
    }
  });
  return found;
}

auto cps_outer_interface::get_ui_child(int control_id, bool add_base_key) -> cgwnd* {
  using find_fn = int(__thiscall*)(const void*, int, int);
  const auto fn = ext_client::off::as_fn<find_fn>(0x009CF790);
  const int result = fn(&ext_client::off::field_at<res_ui_root_map>(this, 0x0B0), control_id, add_base_key ? 1 : 0);
  return result ? reinterpret_cast<cgwnd*>(result) : nullptr;
}
