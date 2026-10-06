#include "pch.hpp"
#include "sdk/process/pcinfo_ui.hpp"

#include "utils/offsets.hpp"

// ===========================================================================
// 1. Identity & Guild (Getters / Setters)
// ===========================================================================
auto pcinfo_ui::get_model_id() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x00);
}

auto pcinfo_ui::get_char_name() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x04);
}

auto pcinfo_ui::get_guild_name() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x20);
}

auto pcinfo_ui::get_guild_name2() const -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x64);
}

auto pcinfo_ui::set_model_id(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x00) = val;
}

auto pcinfo_ui::set_char_name(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x04) = val;
}

auto pcinfo_ui::set_guild_name(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x20) = val;
}

auto pcinfo_ui::set_guild_name2(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x64) = val;
}

// ===========================================================================
// 2. Character Progression & Stats (Getters / Setters)
// ===========================================================================
auto pcinfo_ui::get_level() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x3C);
}

auto pcinfo_ui::get_style() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x3D);
}

auto pcinfo_ui::get_style2() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x60);
}

auto pcinfo_ui::get_exp() const -> std::uint64_t {
  return ext_client::off::field_at<std::uint64_t>(this, 0x40);
}

auto pcinfo_ui::get_strength() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x48);
}

auto pcinfo_ui::get_intelligence() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x4A);
}

auto pcinfo_ui::get_stat_points() const -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x4C);
}

auto pcinfo_ui::get_hp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x50);
}

auto pcinfo_ui::get_mp() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x54);
}

auto pcinfo_ui::set_level(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x3C) = val;
}

auto pcinfo_ui::set_style(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x3D) = val;
}

auto pcinfo_ui::set_style2(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x60) = val;
}

auto pcinfo_ui::set_exp(std::uint64_t val) -> void {
  ext_client::off::field_at<std::uint64_t>(this, 0x40) = val;
}

auto pcinfo_ui::set_strength(std::uint16_t val) -> void {
  ext_client::off::field_at<std::uint16_t>(this, 0x48) = val;
}

auto pcinfo_ui::set_intelligence(std::uint16_t val) -> void {
  ext_client::off::field_at<std::uint16_t>(this, 0x4A) = val;
}

auto pcinfo_ui::set_stat_points(std::uint16_t val) -> void {
  ext_client::off::field_at<std::uint16_t>(this, 0x4C) = val;
}

auto pcinfo_ui::set_hp(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x50) = val;
}

auto pcinfo_ui::set_mp(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x54) = val;
}

// ===========================================================================
// 3. Timers & Deletion State
// ===========================================================================
auto pcinfo_ui::get_deleting() const -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x58);
}

auto pcinfo_ui::get_deletion_time() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x5C);
}

auto pcinfo_ui::get_packed_time() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0xFC);
}

auto pcinfo_ui::set_deleting(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x58) = val;
}

auto pcinfo_ui::set_deletion_time(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x5C) = val;
}

auto pcinfo_ui::set_packed_time(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0xFC) = val;
}

// ===========================================================================
// 4. Equipment & Visual Properties
// ===========================================================================
auto pcinfo_ui::get_avatar_ids() const -> ext_client::msvc9::vector<std::uint32_t> {
  return ext_client::off::field_at<ext_client::msvc9::n_vector<std::uint32_t>>(this, 0xEC);
}

auto pcinfo_ui::get_blend_var1() const -> blend_variable<std::uint8_t> {
  return blend_variable<std::uint8_t>{};
}

auto pcinfo_ui::get_blend_var2() const -> blend_variable<std::uint8_t> {
  return blend_variable<std::uint8_t>{};
}

auto pcinfo_ui::get_font_texture() const -> cg_font_texture {
  return cg_font_texture{};
}

auto pcinfo_ui::set_avatar_ids(ext_client::msvc9::vector<std::uint32_t> val) -> void {
  ext_client::off::field_at<ext_client::msvc9::n_vector<std::uint32_t>>(this, 0xEC) = val;
}

auto pcinfo_ui::set_blend_var1(blend_variable<std::uint8_t> val) -> void {
  (void)val;
}

auto pcinfo_ui::set_blend_var2(blend_variable<std::uint8_t> val) -> void {
  (void)val;
}

auto pcinfo_ui::set_font_texture(cg_font_texture val) -> void {
  (void)val;
}
