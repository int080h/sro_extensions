#include "pch.hpp"
#include "sdk/ui/cif_main_popup.hpp"

#include "sdk/render/cg_interface.hpp"
#include "utils/offsets.hpp"

namespace {
  using ext_client::off::as_fn;
}

// alram_entry methods
auto alram_entry::is_active() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x028) != 0;
}

auto alram_entry::get_type() const -> int {
  using get_type_fn = int(__thiscall*)(const alram_entry* self);
  const auto fn = as_fn<get_type_fn>(0x009D5C50);
  return fn(this);
}

auto alram_entry::is_facebook() const -> bool {
  const int t = get_type();
  return t == 6/*facebook*/ || t == 62/*rigid_facebook*/;
}

auto alram_entry::is_magic_lamp() const -> bool {
  const int t = get_type();
  return t == 7/*magic_lamp*/ || t == 46/*rigid_magic_lamp*/;
}

auto alram_entry::is_daily_login() const -> bool {
  return get_type() == 8/*daily_login*/;
}

auto alram_entry::is_hidden() const -> bool {
  return is_facebook() || is_magic_lamp() || is_daily_login();
}

auto alram_entry::set_active(bool active) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x028) = active ? 1 : 0;
}

auto alram_entry::set_type(int type) -> void {
  ext_client::off::field_at<int>(this, 0x098) = type;
}

auto alram_entry::get_ref_data_ptr() -> void* {
  using ref_data_ptr_fn = int(__thiscall*)(alram_entry * self);
  const auto fn = as_fn<ref_data_ptr_fn>(0x009D5BF0);
  return reinterpret_cast<void*>(fn(this));
}

// alram_data methods
auto alram_data::get_entry(std::size_t index) -> alram_entry* {
  using entry_at_fn = alram_entry*(__thiscall*)(alram_data * self, int index);
  const auto fn = as_fn<entry_at_fn>(0x0078BAD0);
  return fn(this, static_cast<int>(index));
}

auto alram_data::get_entry(std::size_t index) const -> const alram_entry* {
  return const_cast<alram_data*>(this)->get_entry(index);
}

auto alram_data::get_entry_at_raw(alram_data* data, std::size_t index) -> alram_entry* {
  if (!data || index >= calram_entry_count) {
    return nullptr;
  }
  return reinterpret_cast<alram_entry*>(reinterpret_cast<std::uint8_t*>(data) + 0x2450 + index * 0x1F8);
}

auto alram_data::get_entry_at_raw(const alram_data* data, std::size_t index) -> const alram_entry* {
  return get_entry_at_raw(const_cast<alram_data*>(data), index);
}

// cif_main_popup methods
auto cif_main_popup::get_alram() -> alram_data* {
  using get_alram_fn = alram_data*(__thiscall*)(cif_main_popup * self);
  const auto fn = as_fn<get_alram_fn>(0x00781E50);
  return fn(this);
}

auto cif_main_popup::get_alram() const -> const alram_data* {
  return const_cast<cif_main_popup*>(this)->get_alram();
}

auto cif_main_popup::from_interface(cg_interface* iface) -> cif_main_popup* {
  if (!iface) {
    return nullptr;
  }
  using from_interface_fn = cif_main_popup*(__thiscall*)(void* cg_interface);
  const auto fn = as_fn<from_interface_fn>(0x008831F0);
  return reinterpret_cast<cif_main_popup*>(fn(iface));
}
