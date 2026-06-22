#include "cps_outer_interface.hpp"

#include "cgwnd.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

auto cps_outer_interface::find_child(int res_id) -> cgwnd* {
  if (res_id < 0) {
    return nullptr;
  }

  if (auto* widget = static_cast<cgwnd*>(get_ui_child(res_id, true))) {
    if (widget && widget->is_live()) {
      return widget;
    }
  }

  if (auto* widget = cgwnd::get_child_by_unique_id(reinterpret_cast<cgwnd*>(this), res_id)) {
    if (widget && widget->is_live()) {
      return widget;
    }
  }

  return nullptr;
}

auto cps_outer_interface::res_map_key_for(const cgwnd* widget) -> int {
  if (!widget) {
    return -1;
  }

  const auto* map = res_ui_root();
  if (!map) {
    return -1;
  }

  int found = -1;
  map->for_each([&](int key, void* value) {
    if (value == widget) {
      found = key;
    }
  });
  return found;
}

auto cps_outer_interface::get_ui_child(int control_id, bool add_base_key) -> void* {
  using find_fn = int(__thiscall*)(const void*, int, int);
  const auto fn = ext_client::offsets::as_fn<find_fn>(ext_client::offsets::cres_id_manager::functions::find);
  const int result = fn(&m_res_ui_root, control_id, add_base_key ? 1 : 0);
  return result ? reinterpret_cast<void*>(result) : nullptr;
}
