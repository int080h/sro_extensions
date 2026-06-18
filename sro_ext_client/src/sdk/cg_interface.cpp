#include "cg_interface.hpp"

#include "calarm_store.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto cg_interface::get() -> cg_interface* {
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::globals::cg_interface), sizeof(void*))) {
    return nullptr;
  }
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
  if (!ptr ||
      !ext_client::msvc9::is_readable_ptr(ptr, ext_client::offsets::cg_interface::fields::textboard_vftable + sizeof(std::uint32_t))) {
    return false;
  }
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(ptr);
  const auto primary = *reinterpret_cast<const std::uint32_t*>(bytes);
  if (primary != ext_client::offsets::cg_interface::vtable::address) {
    return false;
  }
  const auto secondary =
      *reinterpret_cast<const std::uint32_t*>(bytes + ext_client::offsets::cg_interface::fields::textboard_vftable);
  return secondary == ext_client::offsets::cg_interface::vtable::secondary;
}

auto cg_interface::ui_res_map() -> cres_id_manager* {
  return cres_id_manager::from(reinterpret_cast<std::uint8_t*>(this) + ext_client::offsets::cg_interface::fields::ui_res_map);
}

auto cg_interface::ui_res_map() const -> const cres_id_manager* {
  return cres_id_manager::from(reinterpret_cast<const std::uint8_t*>(this) + ext_client::offsets::cg_interface::fields::ui_res_map);
}

auto cg_interface::textboard() -> ctextboard* {
  return reinterpret_cast<ctextboard*>(reinterpret_cast<std::uint8_t*>(this) +
                                       ext_client::offsets::cg_interface::fields::textboard_vftable);
}

auto cg_interface::textboard() const -> const ctextboard* {
  return const_cast<cg_interface*>(this)->textboard();
}

auto cg_interface::get_ui_child(int control_id, bool add_base_key) -> void* {
  const auto* map = ui_res_map();
  return map ? map->find(control_id, add_base_key) : nullptr;
}

auto cg_interface::alarm_store() -> calarm_store* {
  return calarm_store::from_interface(this);
}

auto cg_interface::alarm_store() const -> const calarm_store* {
  return const_cast<cg_interface*>(this)->alarm_store();
}

auto cg_interface::alarm_guide_mgr() -> calarm_guide_mgr_wnd* {
  using alarm_guide_mgr_child_fn = calarm_guide_mgr_wnd*(__thiscall*)(cg_interface * self);
  const auto fn = as_fn<alarm_guide_mgr_child_fn>(ext_client::offsets::cg_interface::functions::alarm_guide_mgr_child);
  return fn(this);
}

auto cg_interface::alarm_guide_mgr() const -> const calarm_guide_mgr_wnd* {
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
  if (!ext_client::msvc9::is_readable_ptr(field, sizeof(void*))) {
    return nullptr;
  }
  const auto* popup = *field;
  return ext_client::msvc9::is_game_ptr(popup) ? const_cast<void*>(popup) : nullptr;
}

auto cg_interface::modal_wnd() const -> void* {
  const auto* field = field_ptr<const void*>(ext_client::offsets::cg_interface::fields::modal_wnd);
  if (!ext_client::msvc9::is_readable_ptr(field, sizeof(void*))) {
    return nullptr;
  }
  const auto* wnd = *field;
  return ext_client::msvc9::is_game_ptr(wnd) ? const_cast<void*>(wnd) : nullptr;
}

auto cg_interface::textboard_vftable() -> ctextboard_vtable* {
  return m_textboard_vftable;
}

auto cg_interface::textboard_vftable() const -> const ctextboard_vtable* {
  return m_textboard_vftable;
}

