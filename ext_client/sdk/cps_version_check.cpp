#include "cps_version_check.hpp"

namespace {

  cps_version_check* g_current_instance = nullptr;

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

void cps_version_check::set_current(cps_version_check* instance) {
  g_current_instance = instance;
}

auto cps_version_check::current() -> cps_version_check* {
  return g_current_instance;
}

auto cps_version_check::is_active() -> bool {
  return global_at<int>(ext_client::offsets::cps_version_check::globals::version_check_active) != 0;
}

auto cps_version_check::version_error_code() -> int {
  return global_at<int>(ext_client::offsets::cps_version_check::globals::version_error_code);
}

auto cps_version_check::version_error_tag() -> int {
  return global_at<int>(ext_client::offsets::cps_version_check::globals::version_error_tag);
}

auto cps_version_check::create() -> cps_version_check* {
  using create_instance_fn = int(__cdecl*)();
  const auto fn = as_fn<create_instance_fn>(ext_client::offsets::cps_version_check::functions::create_instance);
  return reinterpret_cast<cps_version_check*>(fn());
}

auto cps_version_check::connect_gateway() -> bool {
  using connect_gateway_fn = int(__cdecl*)();
  const auto fn = as_fn<connect_gateway_fn>(ext_client::offsets::cps_version_check::functions::connect_gateway);
  return fn() != 0;
}

auto cps_version_check::load_game_textdata(void* cg_interface) -> bool {
  using load_textdata_fn = char(__thiscall*)(void* cg_interface);
  const auto fn = as_fn<load_textdata_fn>(ext_client::offsets::cps_version_check::functions::load_game_textdata);
  return fn(cg_interface) != 0;
}

auto cps_version_check::set_version_active(bool active) -> void {
  using set_version_active_fn = int(__stdcall*)(int active);
  const auto fn = as_fn<set_version_active_fn>(ext_client::offsets::cps_version_check::functions::set_version_active);
  fn(active ? 1 : 0);
}

auto cps_version_check::data_load_started() const -> bool {
  return m_data_load_started != 0;
}

auto cps_version_check::set_data_load_started(bool value) -> void {
  m_data_load_started = value ? 1 : 0;
}
