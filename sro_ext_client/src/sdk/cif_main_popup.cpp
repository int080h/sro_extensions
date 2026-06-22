#include "cif_main_popup.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::offsets::as_fn;
}

// alram_entry methods
auto alram_entry::active() const -> bool {
  return m_active != 0;
}

auto alram_entry::type() const -> int {
  using get_type_fn = int(__thiscall*)(const alram_entry* self);
  const auto fn = as_fn<get_type_fn>(ext_client::offsets::calarm_entry::functions::get_type);
  return fn(this);
}

auto alram_entry::is_facebook() const -> bool {
  const int t = type();
  return t == ext_client::offsets::calarm_entry::types::facebook || t == ext_client::offsets::calarm_entry::types::rigid_facebook;
}

auto alram_entry::is_magic_lamp() const -> bool {
  const int t = type();
  return t == ext_client::offsets::calarm_entry::types::magic_lamp || t == ext_client::offsets::calarm_entry::types::rigid_magic_lamp;
}

auto alram_entry::is_daily_login() const -> bool {
  return type() == ext_client::offsets::calarm_entry::types::daily_login;
}

auto alram_entry::is_hidden() const -> bool {
  return is_facebook() || is_magic_lamp() || is_daily_login();
}

auto alram_entry::set_active(bool active) -> void {
  m_active = active ? 1 : 0;
}

auto alram_entry::set_type(int type) -> void {
  m_type = type;
}

auto alram_entry::ref_data_ptr() -> void* {
  using ref_data_ptr_fn = int(__thiscall*)(alram_entry * self);
  const auto fn = as_fn<ref_data_ptr_fn>(ext_client::offsets::calarm_entry::functions::ref_data_ptr);
  return reinterpret_cast<void*>(fn(this));
}

// alram_data methods
auto alram_data::entry(std::size_t index) -> alram_entry* {
  using entry_at_fn = alram_entry*(__thiscall*)(alram_data * self, int index);
  const auto fn = as_fn<entry_at_fn>(ext_client::offsets::calarm_data::functions::entry_at);
  return fn(this, static_cast<int>(index));
}

auto alram_data::entry(std::size_t index) const -> const alram_entry* {
  return const_cast<alram_data*>(this)->entry(index);
}

auto alram_data::entry_at_raw(alram_data* data, std::size_t index) -> alram_entry* {
  if (!data || index >= calram_entry_count) {
    return nullptr;
  }
  return &data->m_entries[index];
}

auto alram_data::entry_at_raw(const alram_data* data, std::size_t index) -> const alram_entry* {
  return entry_at_raw(const_cast<alram_data*>(data), index);
}

// cif_main_popup methods
auto cif_main_popup::get_alram() -> alram_data* {
  using get_alram_fn = alram_data*(__thiscall*)(cif_main_popup * self);
  const auto fn = as_fn<get_alram_fn>(ext_client::offsets::calarm_store::functions::alarm_data_ptr);
  return fn(this);
}

auto cif_main_popup::get_alram() const -> const alram_data* {
  return const_cast<cif_main_popup*>(this)->get_alram();
}

auto cif_main_popup::from_interface(void* cg_interface) -> cif_main_popup* {
  if (!cg_interface) {
    return nullptr;
  }
  using from_interface_fn = cif_main_popup*(__thiscall*)(void* cg_interface);
  const auto fn = as_fn<from_interface_fn>(ext_client::offsets::calarm_store::functions::from_interface);
  return reinterpret_cast<cif_main_popup*>(fn(cg_interface));
}
