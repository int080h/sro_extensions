#include "cg_interface.hpp"

#include "cif_main_popup.hpp"
#include "calram_guide_mgr_wnd.hpp"
#include "utils/rtti.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto cg_interface::get() -> cg_interface* {
  return global_at<cg_interface*>(ext_client::offsets::globals::cg_interface);
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
  return iface->get_ui_child(ext_client::offsets::cg_interface::child_ids::main_hud, true) != nullptr;
}

auto cg_interface::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::msvc9::is_game_ptr(ptr)) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(ptr, "CGInterface");
}

auto cg_interface::textboard() -> ctextboard* {
  return reinterpret_cast<ctextboard*>(reinterpret_cast<std::uint8_t*>(this) +
                                       ext_client::offsets::cg_interface::fields::textboard_vftable);
}

auto cg_interface::textboard() const -> const ctextboard* {
  return const_cast<cg_interface*>(this)->textboard();
}

auto cg_interface::get_ui_child(int control_id, bool add_base_key) -> void* {
  using find_fn = int(__thiscall*)(const void*, int, int);
  const auto fn = as_fn<find_fn>(ext_client::offsets::cres_id_manager::functions::find);
  const int result = fn(&m_ui_res_map, control_id, add_base_key ? 1 : 0);
  return result ? reinterpret_cast<void*>(result) : nullptr;
}

auto cg_interface::alarm_store() -> cif_main_popup* {
  return cif_main_popup::from_interface(this);
}

auto cg_interface::alarm_store() const -> const cif_main_popup* {
  return const_cast<cg_interface*>(this)->alarm_store();
}

auto cg_interface::alarm_guide_mgr() -> calram_guide_mgr_wnd* {
  using alarm_guide_mgr_child_fn = calram_guide_mgr_wnd*(__thiscall*)(cg_interface * self);
  const auto fn = as_fn<alarm_guide_mgr_child_fn>(ext_client::offsets::cg_interface::functions::alarm_guide_mgr_child);
  return fn(this);
}

auto cg_interface::alarm_guide_mgr() const -> const calram_guide_mgr_wnd* {
  return const_cast<cg_interface*>(this)->alarm_guide_mgr();
}

auto cg_interface::guide_host(bool add_base_key) -> void* {
  return get_ui_child(ext_client::offsets::cg_interface::child_ids::guide_host, add_base_key);
}

auto cg_interface::show_magic_lamp_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(ext_client::offsets::cg_interface::functions::show_magic_lamp_guide);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_daily_login_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(ext_client::offsets::cg_interface::functions::show_daily_login_guide);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_facebook_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(ext_client::offsets::cg_interface::functions::show_facebook_guide);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_web_item_alarm_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(ext_client::offsets::cg_interface::functions::show_web_item_alarm_guide);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::show_macro_guide(bool show) -> unsigned int {
  using show_guide_fn = unsigned int(__thiscall*)(cg_interface * self, char show);
  const auto fn = as_fn<show_guide_fn>(ext_client::offsets::cg_interface::functions::show_macro_guide);
  return fn(this, show ? 1 : 0);
}

auto cg_interface::known_child_id_count() -> std::size_t {
  return ext_client::offsets::cg_interface::child_ids::known_count;
}

auto cg_interface::known_child_id(std::size_t index) -> int {
  if (index >= known_child_id_count()) {
    return 0;
  }
  return ext_client::offsets::cg_interface::child_ids::known[index];
}

auto cg_interface::alarm_guide_mgr_popup() const -> void* {
  const auto* field = field_ptr<const void*>(ext_client::offsets::cg_interface::fields::alarm_guide_mgr_popup);
  if (!field) {
    return nullptr;
  }
  return const_cast<void*>(*field);
}

auto cg_interface::modal_wnd() const -> void* {
  const auto* field = field_ptr<const void*>(ext_client::offsets::cg_interface::fields::modal_wnd);
  if (!field) {
    return nullptr;
  }
  return const_cast<void*>(*field);
}

auto cg_interface::textboard_vftable() -> ctextboard_vtable* {
  return m_textboard_vftable;
}

auto cg_interface::textboard_vftable() const -> const ctextboard_vtable* {
  return m_textboard_vftable;
}

