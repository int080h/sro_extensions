#include "pch.hpp"
#include "sdk/process/cps_version_check.hpp"

#include "sdk/game/ccontroler.hpp"
#include "sdk/ui/cif_static.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cps_version_check::current() -> cps_version_check* {
  return ccontroler::active_child_as<cps_version_check>("CPSVersionCheck");
}

auto cps_version_check::is_active() -> bool {
  return global_at<int>(0x0117E154) != 0;
}

auto cps_version_check::version_error_code() -> int {
  return global_at<int>(0x0117E8A0);
}

auto cps_version_check::version_error_tag() -> int {
  return global_at<int>(0x0117E89C);
}

auto cps_version_check::create() -> cps_version_check* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(0x00978C10);
  return reinterpret_cast<cps_version_check*>(fn());
}

auto cps_version_check::connect_gateway() -> bool {
  using connect_gateway_fn = int(__cdecl*)();
  const auto fn = as_fn<connect_gateway_fn>(0x00942690);
  return fn() != 0;
}

auto cps_version_check::load_game_textdata(void* cg_interface) -> bool {
  using load_textdata_fn = char(__thiscall*)(void* cg_interface);
  const auto fn = as_fn<load_textdata_fn>(0x00943C10);
  return fn(cg_interface) != 0;
}

auto cps_version_check::set_version_active(bool active) -> void {
  using set_version_active_fn = int(__stdcall*)(int active);
  const auto fn = as_fn<set_version_active_fn>(0x00949B80);
  fn(active ? 1 : 0);
}

auto cps_version_check::is_data_load_started() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x10C) != 0;
}

auto cps_version_check::set_data_load_started(bool value) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x10C) = value ? 1 : 0;
}

auto cps_version_check::find_loading_banner_widget() -> cif_static* {
  auto* root = reinterpret_cast<cgwnd*>(this);
  if (!root) {
    return nullptr;
  }

  cif_static* found = nullptr;
  if (!ext_client::off::field_at<ext_client::msvc9::n_list<cgwnd*>>(root, 0x078).empty()) {
    ext_client::off::field_at<ext_client::msvc9::n_list<cgwnd*>>(root, 0x078).for_each([&](cgwnd* child) {
      if (child) {
        const std::uint32_t vft = *reinterpret_cast<std::uint32_t*>(child);
        if (vft == 0x00FF46BC) {
          found = reinterpret_cast<cif_static*>(child);
        }
      }
    });
  }
  return found;
}
