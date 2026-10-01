#include "pch.hpp"
#include "sdk/process/cps_character_select.hpp"

#include "sdk/game/ccontroler.hpp"
#include "utils/msvc9_stl.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;

} // namespace

auto cps_character_select::get_current() -> cps_character_select* {
  return get_resolve_live();
}

auto cps_character_select::is_live(const void* ptr) -> bool {
  return ccontroler::is_process(ptr, "CPSCharacterSelect");
}

auto cps_character_select::create() -> cps_character_select* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00963C00);
  return reinterpret_cast<cps_character_select*>(fn());
}

auto cps_character_select::get_resolve_live() -> cps_character_select* {
  return ccontroler::active_child_as<cps_character_select>("CPSCharacterSelect");
}



auto cps_character_select::get_selected_slot_index() -> int {
  return static_cast<int>(global_at<std::uint8_t>(0x01152638));
}

auto cps_character_select::is_pin_required() -> bool {
  return global_at<int>(0x0117E3D0) != 0;
}

auto cps_character_select::request_character_list() -> void {
  using request_char_list_fn = int(__cdecl*)();
  const auto fn = as_fn<request_char_list_fn>(0x00957CC0);
  fn();
}

auto cps_character_select::get_char_count() const -> int {
  return ext_client::off::field_at<std::uint8_t>(this, 0x12C);
}

auto cps_character_select::get_page_index() const -> int {
  return ext_client::off::field_at<std::uint8_t>(this, 0x12D);
}

auto cps_character_select::get_page_count() const -> int {
  return ext_client::off::field_at<std::uint8_t>(this, 0x12E);
}

auto cps_character_select::get_selected_slot() const -> int {
  return static_cast<int>(static_cast<signed char>(ext_client::off::field_at<std::uint8_t>(this, 0x130)));
}

auto cps_character_select::get_character_at(int index) const -> pcinfo_ui* {
  if (index < 0 || index >= get_char_count()) {
    return nullptr;
  }
  const auto& chars = const_cast<cps_character_select*>(this)->get_characters();
  if (index >= static_cast<int>(chars.size())) {
    return nullptr;
  }
  const auto base = reinterpret_cast<std::uintptr_t>(chars[index]);
  if (!base) {
    return nullptr;
  }
  return reinterpret_cast<pcinfo_ui*>(base + 0x8C0);
}

auto cps_character_select::get_deco_character_at(int index) const -> cic_deco_character* {
  if (index < 0 || index >= get_char_count()) {
    return nullptr;
  }
  const auto& chars = const_cast<cps_character_select*>(this)->get_characters();
  if (index >= static_cast<int>(chars.size())) {
    return nullptr;
  }
  return reinterpret_cast<cic_deco_character*>(chars[index]);
}

auto cps_character_select::handle_character_list(void* packet) -> int {
  if (!packet) {
    return 0;
  }
  using handle_char_list_fn = int(__thiscall*)(cps_character_select * self, void* packet);
  const auto fn = as_fn<handle_char_list_fn>(0x00962170);
  return fn(this, packet);
}

auto cps_character_select::begin_enter_world_fade() -> int {
  using enter_world_fade_fn = int(__cdecl*)(cps_character_select * self);
  const auto fn = as_fn<enter_world_fade_fn>(0x0095D550);
  return fn(this);
}

auto cps_character_select::show_delete_dialog(bool show) -> int {
  using show_delete_dialog_fn = int(__thiscall*)(cps_character_select * self, char show);
  const auto fn = as_fn<show_delete_dialog_fn>(0x0095A6D0);
  return fn(this, show ? 1 : 0);
}

auto cps_character_select::init_defaults() -> int {
  using init_defaults_fn = int(__cdecl*)(cps_character_select * self);
  const auto fn = as_fn<init_defaults_fn>(0x0095C9B0);
  return fn(this);
}

auto cps_character_select::set_login_phase(int phase) -> void {
  ext_client::off::field_at<int>(this, 0x0E4) = phase;
}

auto cps_character_select::get_characters() -> ext_client::msvc9::n_vector<s_character_info*>& {
  return ext_client::off::field_at<ext_client::msvc9::n_vector<s_character_info*>>(this, 0x134);
}

auto cps_character_select::get_char_objects() -> ext_client::msvc9::n_vector<void*>& {
  return ext_client::off::field_at<ext_client::msvc9::n_vector<void*>>(this, 0x144);
}

auto cps_character_select::get_ui_map() -> ext_client::msvc9::n_map<int, void*>& {
  return ext_client::off::field_at<ext_client::msvc9::n_map<int, void*>>(this, 0x154);
}
