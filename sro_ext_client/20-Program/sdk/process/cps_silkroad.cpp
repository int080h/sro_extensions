#include "pch.hpp"
#include "sdk/process/cps_silkroad.hpp"

#include "utils/offsets.hpp"

auto cps_silkroad::get_res_ui_root() -> res_ui_root_map& {
  return ext_client::off::field_at<res_ui_root_map>(this, 0x0B0);
}

auto cps_silkroad::get_res_ui_root() const -> const res_ui_root_map& {
  return ext_client::off::field_at<res_ui_root_map>(this, 0x0B0);
}

auto cps_silkroad::get_res_loader() -> void* {
  return ext_client::off::field_at<void*>(this, 0x0E0);
}

auto cps_silkroad::get_login_phase() -> int {
  return ext_client::off::field_at<int>(this, 0x0E4);
}

auto cps_silkroad::get_login_mode() -> int {
  return ext_client::off::field_at<int>(this, 0x0E8);
}

auto cps_silkroad::set_res_loader(void* val) -> void {
  ext_client::off::field_at<void*>(this, 0x0E0) = val;
}

auto cps_silkroad::set_login_phase(int val) -> void {
  ext_client::off::field_at<int>(this, 0x0E4) = val;
}

auto cps_silkroad::set_login_mode(int val) -> void {
  ext_client::off::field_at<int>(this, 0x0E8) = val;
}
