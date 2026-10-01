#include "pch.hpp"
#include "sdk/game/ci_object.hpp"
#include "utils/offsets.hpp"

auto ci_object::get_alpha_fade() -> float {
  return ext_client::off::field_at<float>(this, 0x8C);
}

auto ci_object::get_coords() -> s_position {
  return ext_client::off::field_at<s_position>(this, 0x7C);
}

auto ci_object::get_bound_min_x() -> float {
  return ext_client::off::field_at<float>(this, 0xA0);
}

auto ci_object::get_bound_min_y() -> float {
  return ext_client::off::field_at<float>(this, 0xA4);
}

auto ci_object::get_bound_min_z() -> float {
  return ext_client::off::field_at<float>(this, 0xA8);
}

auto ci_object::get_bound_max_x() -> float {
  return ext_client::off::field_at<float>(this, 0xAC);
}

auto ci_object::get_bound_max_y() -> float {
  return ext_client::off::field_at<float>(this, 0xB0);
}

auto ci_object::get_bound_max_z() -> float {
  return ext_client::off::field_at<float>(this, 0xB4);
}

auto ci_object::get_rendering_mode() -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0xBC);
}

auto ci_object::get_scale_x() -> float {
  return ext_client::off::field_at<float>(this, 0xC0);
}

auto ci_object::get_scale_y() -> float {
  return ext_client::off::field_at<float>(this, 0xC4);
}

auto ci_object::get_scale_z() -> float {
  return ext_client::off::field_at<float>(this, 0xC8);
}

auto ci_object::set_alpha_fade(float val) -> void {
  ext_client::off::field_at<float>(this, 0x8C) = val;
}

auto ci_object::set_coords(s_position val) -> void {
  ext_client::off::field_at<s_position>(this, 0x7C) = val;
}

auto ci_object::set_bound_min_x(float val) -> void {
  ext_client::off::field_at<float>(this, 0xA0) = val;
}

auto ci_object::set_bound_min_y(float val) -> void {
  ext_client::off::field_at<float>(this, 0xA4) = val;
}

auto ci_object::set_bound_min_z(float val) -> void {
  ext_client::off::field_at<float>(this, 0xA8) = val;
}

auto ci_object::set_bound_max_x(float val) -> void {
  ext_client::off::field_at<float>(this, 0xAC) = val;
}

auto ci_object::set_bound_max_y(float val) -> void {
  ext_client::off::field_at<float>(this, 0xB0) = val;
}

auto ci_object::set_bound_max_z(float val) -> void {
  ext_client::off::field_at<float>(this, 0xB4) = val;
}

auto ci_object::set_rendering_mode(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0xBC) = val;
}

auto ci_object::set_scale_x(float val) -> void {
  ext_client::off::field_at<float>(this, 0xC0) = val;
}

auto ci_object::set_scale_y(float val) -> void {
  ext_client::off::field_at<float>(this, 0xC4) = val;
}

auto ci_object::set_scale_z(float val) -> void {
  ext_client::off::field_at<float>(this, 0xC8) = val;
}
