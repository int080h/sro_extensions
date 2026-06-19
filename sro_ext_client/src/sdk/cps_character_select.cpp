#include "cps_character_select.hpp"

#include "live_instance.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/process.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

void cps_character_select::set_current(cps_character_select* instance) {
  ext_client::sdk::live_instance<cps_character_select>::set(instance);
}

auto cps_character_select::current() -> cps_character_select* {
  return ext_client::sdk::live_instance<cps_character_select>::get();
}

auto cps_character_select::is_live(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  std::uint32_t vft = 0;
  if (!ext_client::msvc9::try_read_u32(ptr, &vft)) {
    return false;
  }
  return vft == ext_client::offsets::cps_character_select::vtable::address;
}

auto cps_character_select::create() -> cps_character_select* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(ext_client::offsets::cps_character_select::functions::create_instance);
  return reinterpret_cast<cps_character_select*>(fn());
}

auto cps_character_select::resolve_live() -> cps_character_select* {
  return ext_client::utils::process::active_child_as<cps_character_select>(
    ext_client::offsets::cps_character_select::vtable::address);
}

auto cps_character_select::sync_current() -> cps_character_select* {
  auto* live = resolve_live();
  cps_character_select::set_current(live);
  return live;
}

auto cps_character_select::selected_slot_index() -> int {
  return static_cast<int>(global_at<std::uint8_t>(ext_client::offsets::cps_character_select::globals::selected_slot_index));
}

auto cps_character_select::pin_required() -> bool {
  return global_at<int>(ext_client::offsets::cps_character_select::globals::pin_required_flag) != 0;
}

auto cps_character_select::request_character_list() -> void {
  using request_char_list_fn = int(__cdecl*)();
  const auto fn = as_fn<request_char_list_fn>(ext_client::offsets::cps_character_select::functions::request_char_list);
  fn();
}

auto cps_character_select::char_count() const -> int {
  return m_char_count;
}

auto cps_character_select::page_index() const -> int {
  return m_page_index;
}

auto cps_character_select::page_count() const -> int {
  return m_page_count;
}

auto cps_character_select::selected_slot() const -> int {
  return static_cast<int>(static_cast<signed char>(m_selected_slot));
}

auto cps_character_select::character_at(int index) const -> pcinfo_ui* {
  if (index < 0 || index >= char_count()) {
    return nullptr;
  }
  const auto begin = m_characters_begin;
  const auto end = m_characters_end;
  if (!begin || !end) {
    return nullptr;
  }
  const auto count = static_cast<int>((reinterpret_cast<std::uintptr_t>(end) - reinterpret_cast<std::uintptr_t>(begin)) / sizeof(void*));
  if (index >= count) {
    return nullptr;
  }
  auto* info = static_cast<s_character_info**>(begin)[index];
  return info; // automatically shifts pointer by 0x8C0 to point to pcinfo_ui base
}

auto cps_character_select::handle_character_list(void* packet) -> int {
  if (!packet) {
    return 0;
  }
  using handle_char_list_fn = int(__thiscall*)(cps_character_select * self, void* packet);
  const auto fn = as_fn<handle_char_list_fn>(ext_client::offsets::cps_character_select::functions::handle_char_list);
  return fn(this, packet);
}

auto cps_character_select::begin_enter_world_fade() -> int {
  using enter_world_fade_fn = int(__cdecl*)(cps_character_select * self);
  const auto fn = as_fn<enter_world_fade_fn>(ext_client::offsets::cps_character_select::functions::enter_world_fade);
  return fn(this);
}

auto cps_character_select::show_delete_dialog(bool show) -> int {
  using show_delete_dialog_fn = int(__thiscall*)(cps_character_select * self, char show);
  const auto fn = as_fn<show_delete_dialog_fn>(ext_client::offsets::cps_character_select::functions::show_delete_dialog);
  return fn(this, show ? 1 : 0);
}

auto cps_character_select::init_defaults() -> int {
  using init_defaults_fn = int(__cdecl*)(cps_character_select * self);
  const auto fn = as_fn<init_defaults_fn>(ext_client::offsets::cps_character_select::functions::init_defaults);
  return fn(this);
}

auto cps_character_select::set_login_phase(int phase) -> void {
  m_login_phase = phase;
}
