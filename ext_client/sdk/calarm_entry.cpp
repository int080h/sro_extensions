#include "calarm_entry.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto calarm_entry::active() const -> bool {
  return m_active != 0;
}

auto calarm_entry::type() const -> int {
  using get_type_fn = int(__thiscall*)(const calarm_entry* self);
  const auto fn = as_fn<get_type_fn>(ext_client::offsets::calarm_entry::functions::get_type);
  return fn(this);
}

auto calarm_entry::is_facebook() const -> bool {
  const int t = type();
  return t == ext_client::offsets::calarm_entry::types::facebook || t == ext_client::offsets::calarm_entry::types::rigid_facebook;
}

auto calarm_entry::is_magic_lamp() const -> bool {
  const int t = type();
  return t == ext_client::offsets::calarm_entry::types::magic_lamp || t == ext_client::offsets::calarm_entry::types::rigid_magic_lamp;
}

auto calarm_entry::is_daily_login() const -> bool {
  return type() == ext_client::offsets::calarm_entry::types::daily_login;
}

auto calarm_entry::is_hidden() const -> bool {
  return is_facebook() || is_magic_lamp() || is_daily_login();
}

auto calarm_entry::set_active(bool active) -> void {
  m_active = active ? 1 : 0;
}

auto calarm_entry::set_type(int type) -> void {
  m_type = type;
}

auto calarm_entry::ref_data_ptr() -> void* {
  using ref_data_ptr_fn = int(__thiscall*)(calarm_entry * self);
  const auto fn = as_fn<ref_data_ptr_fn>(ext_client::offsets::calarm_entry::functions::ref_data_ptr);
  return reinterpret_cast<void*>(fn(this));
}
