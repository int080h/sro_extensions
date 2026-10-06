#include "pch.hpp"
#include "sdk/game/ci_object.hpp"

#include "utils/offsets.hpp"

// ===========================================================================
// 1. Position & Coordinates
// ===========================================================================
auto ci_object::get_coords() const -> s_position {
  return ext_client::off::field_at<s_position>(this, 0x7C);
}

auto ci_object::set_coords(const s_position& val) -> void {
  ext_client::off::field_at<s_position>(this, 0x7C) = val;
}

// ===========================================================================
// 2. Bounding Box Dimensions
// ===========================================================================
auto ci_object::get_bound_min_x() const -> float {
  return ext_client::off::field_at<float>(this, 0xA0);
}

auto ci_object::get_bound_min_y() const -> float {
  return ext_client::off::field_at<float>(this, 0xA4);
}

auto ci_object::get_bound_min_z() const -> float {
  return ext_client::off::field_at<float>(this, 0xA8);
}

auto ci_object::get_bound_max_x() const -> float {
  return ext_client::off::field_at<float>(this, 0xAC);
}

auto ci_object::get_bound_max_y() const -> float {
  return ext_client::off::field_at<float>(this, 0xB0);
}

auto ci_object::get_bound_max_z() const -> float {
  return ext_client::off::field_at<float>(this, 0xB4);
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

// ===========================================================================
// 3. Scaling & Rendering
// ===========================================================================
auto ci_object::get_scale_x() const -> float {
  return ext_client::off::field_at<float>(this, 0xC0);
}

auto ci_object::get_scale_y() const -> float {
  return ext_client::off::field_at<float>(this, 0xC4);
}

auto ci_object::get_scale_z() const -> float {
  return ext_client::off::field_at<float>(this, 0xC8);
}

auto ci_object::get_rendering_mode() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0xBC);
}

auto ci_object::get_alpha_fade() const -> float {
  return ext_client::off::field_at<float>(this, 0x8C);
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

auto ci_object::set_rendering_mode(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0xBC) = val;
}

auto ci_object::set_alpha_fade(float val) -> void {
  ext_client::off::field_at<float>(this, 0x8C) = val;
}
