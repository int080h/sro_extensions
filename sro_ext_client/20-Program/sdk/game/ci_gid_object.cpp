#include "pch.hpp"
#include "sdk/game/ci_gid_object.hpp"

#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

// ===========================================================================
// 1. Resource Names & Tags
// ===========================================================================
auto ci_gid_object::get_res_name() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x110);
}

auto ci_gid_object::get_tag_name() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x12C);
}

auto ci_gid_object::get_status_name() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x148);
}

auto ci_gid_object::get_display_name() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x164);
}

auto ci_gid_object::get_gid_tag() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x244);
}

auto ci_gid_object::get_tag_list_size() const -> std::size_t {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x914).length();
}

auto ci_gid_object::get_tag_list_cap() const -> std::size_t {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x914).capacity();
}

auto ci_gid_object::set_res_name(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x110) = val;
}

auto ci_gid_object::set_tag_name(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x12C) = val;
}

auto ci_gid_object::set_status_name(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x148) = val;
}

auto ci_gid_object::set_display_name(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x164) = val;
}

auto ci_gid_object::set_gid_tag(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x244) = val;
}

auto ci_gid_object::set_tag_list_size(std::size_t val) -> void {
  (void)val;
}

auto ci_gid_object::set_tag_list_cap(std::size_t val) -> void {
  (void)val;
}

// ===========================================================================
// 2. View Distance & Fade Timers
// ===========================================================================
auto ci_gid_object::get_view_distance() const -> float {
  return ext_client::off::field_at<float>(this, 0x304);
}

auto ci_gid_object::get_fade_timer() const -> float {
  return ext_client::off::field_at<float>(this, 0x350);
}

auto ci_gid_object::get_fade_speed() const -> float {
  return ext_client::off::field_at<float>(this, 0x354);
}

auto ci_gid_object::set_view_distance(float val) -> void {
  ext_client::off::field_at<float>(this, 0x304) = val;
}

auto ci_gid_object::set_fade_timer(float val) -> void {
  ext_client::off::field_at<float>(this, 0x350) = val;
}

auto ci_gid_object::set_fade_speed(float val) -> void {
  ext_client::off::field_at<float>(this, 0x354) = val;
}
