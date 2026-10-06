#include "pch.hpp"
#include "sdk/render/cg_interface.hpp"

#include "sdk/ui/cif_main_popup.hpp"
#include "sdk/ui/calram_guide_mgr_wnd.hpp"
#include "sdk/ui/cif_target_window.hpp"
#include "sdk/game/ci_charactor.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;

  static constexpr int known_child_ids[] = {
    1, 2, 3, 11, 15, 16, 19, 20, 22, 23, 24, 25, 26, 27, 28, 30, 31, 32, 33, 34,
    35, 36, 39, 40, 41, 42, 43, 44, 45, 47, 49, 50, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 65, 66, 67, 68, 71, 72, 73, 76, 77, 78, 83, 97, 98, 99, 100, 101,
    102, 104, 120, 121, 122, 123, 130, 131, 132, 133, 134, 135, 140, 145, 151,
    153, 154, 155, 156, 157, 158, 167, 270, 300, 301, 302, 303, 304, 314, 410, 885
  };
} // namespace

auto cg_interface::get() -> cg_interface* {
  return global_at<cg_interface*>(0x013BAE3C);
}

auto cg_interface::is_ready() -> bool {
  return get() != nullptr;
}

auto cg_interface::is_ingame_hud_ready() -> bool {
  auto* iface = get();
  if (!iface || !is_instance(iface)) {
    return false;
  }

  // main_hud is the reliable in-world signal (do not use stale cps_title::current()).
  return iface->get_ui_child(1/*main_hud*/, true) != nullptr;
}

auto cg_interface::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::msvc9::is_game_ptr(ptr)) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(ptr, "CGInterface");
}

auto cg_interface::get_textboard() -> ctext_board* {
  return &ext_client::off::field_at<ctext_board>(this, 0x084);
}

auto cg_interface::get_textboard() const -> const ctext_board* {
  return &ext_client::off::field_at<ctext_board>(this, 0x084);
}

auto cg_interface::get_ui_res_map() -> ui_res_map_t* {
  return &ext_client::off::field_at<ui_res_map_t>(this, 0x374);
}

auto cg_interface::get_ui_res_map() const -> const ui_res_map_t* {
  return &ext_client::off::field_at<ui_res_map_t>(this, 0x374);
}

auto cg_interface::walk_each(int max_depth, cgwnd::child_visitor_fn visit, void* ctx) -> void {
  reinterpret_cast<cgwnd*>(this)->walk_each(max_depth, visit, ctx);
}

auto cg_interface::get_ui_child(int control_id, bool add_base_key) -> cgwnd* {
  using find_fn = int(__thiscall*)(const void*, int, int);
  const auto fn = as_fn<find_fn>(0x009CF790);
  const int result = fn(&ext_client::off::field_at<ui_res_map_t>(this, 0x374), control_id, add_base_key ? 1 : 0);
  return result ? reinterpret_cast<cgwnd*>(result) : nullptr;
}

auto cg_interface::get_alarm_store() -> cif_main_popup* {
  return cif_main_popup::from_interface(this);
}

auto cg_interface::get_alarm_store() const -> const cif_main_popup* {
  return const_cast<cg_interface*>(this)->get_alarm_store();
}

auto cg_interface::get_alarm_guide_mgr() -> calram_guide_mgr_wnd* {
  using alarm_guide_mgr_child_fn = calram_guide_mgr_wnd*(__thiscall*)(cg_interface * self);
  const auto fn = as_fn<alarm_guide_mgr_child_fn>(0x008840C0);
  return fn(this);
}

auto cg_interface::get_alarm_guide_mgr() const -> const calram_guide_mgr_wnd* {
  return const_cast<cg_interface*>(this)->get_alarm_guide_mgr();
}

auto cg_interface::get_guide_host(bool add_base_key) -> cgwnd* {
  return get_ui_child(k_child_guide_host, add_base_key);
}

auto cg_interface::show_magic_lamp_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(0x008844C0);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_daily_login_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(0x008844F0);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_facebook_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(0x00884720);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_web_item_alarm_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(0x00883500);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_macro_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(0x00884170);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::known_child_id_count() -> std::size_t {
  return sizeof(known_child_ids) / sizeof(known_child_ids[0]);
}

auto cg_interface::known_child_id(std::size_t index) -> int {
  if (index >= known_child_id_count()) {
    return 0;
  }
  return known_child_ids[index];
}

auto cg_interface::get_alarm_guide_mgr_popup() -> void* {
  return ext_client::off::field_at<void*>(this, 0x738);
}

auto cg_interface::get_alarm_guide_mgr_popup() const -> void* {
  return ext_client::off::field_at<void*>(this, 0x738);
}

auto cg_interface::get_modal_wnd() -> void* {
  return ext_client::off::field_at<void*>(this, 0x808);
}

auto cg_interface::get_modal_wnd() const -> void* {
  return ext_client::off::field_at<void*>(this, 0x808);
}

#include "sdk/game/centity_manager.hpp"

auto cg_interface::target_window() -> cif_target_window* {
  if (!this || !ext_client::utils::memory::is_game_ptr(this)) return nullptr;

  // 1. Check Player target window (offset +0x3B8 / sub_85D5F0)
  auto* player_tw = ext_client::off::field_at<cif_target_window*>(this, 0x3B8);
  if (player_tw && ext_client::utils::memory::is_game_ptr(player_tw) && player_tw->is_visible() && player_tw->target_slot_id() > 0) {
    return player_tw;
  }

  // 2. Check Monster/NPC target window (offset +0x3BC / sub_85D600)
  auto* monster_tw = ext_client::off::field_at<cif_target_window*>(this, 0x3BC);
  if (monster_tw && ext_client::utils::memory::is_game_ptr(monster_tw) && monster_tw->is_visible() && monster_tw->target_slot_id() > 0) {
    return monster_tw;
  }

  // Fallback to active sub_85D600 call
  using get_target_window_fn = cif_target_window*(__thiscall*)(cg_interface*);
  const auto fn = ext_client::off::as_fn<get_target_window_fn>(0x0085D600);
  auto* tw = fn ? fn(this) : nullptr;
  if (tw && ext_client::utils::memory::is_game_ptr(tw) && tw->is_visible()) {
    return tw;
  }

  return nullptr;
}

auto cg_interface::target_window() const -> const cif_target_window* {
  return const_cast<cg_interface*>(this)->target_window();
}

auto cg_interface::has_target() const -> bool {
  auto* tw = const_cast<cg_interface*>(this)->target_window();
  return tw && ext_client::utils::memory::is_game_ptr(tw) && tw->is_visible() && tw->target_slot_id() > 0;
}

auto cg_interface::target_entity() const -> ci_charactor* {
  auto* tw = const_cast<cg_interface*>(this)->target_window();
  if (tw && ext_client::utils::memory::is_game_ptr(tw) && tw->is_visible()) {
    const auto slot_id = tw->target_slot_id();
    if (slot_id > 0) {
      return centity_manager::resolve_by_uid_or_slot(slot_id);
    }
  }
  return nullptr;
}
