#include "pch.hpp"
#include "sdk/types/pcinfo_ui.hpp"
#include "utils/offsets.hpp"

// pcinfo_ui: UI-side character info (name, level, stats, appearance).
// Embedded in s_character_info at offset 0x8C0 (after cic_deco_character).
// Field layout verified from character list packet parser (sub_961DF0).

auto pcinfo_ui::get_model_id() -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x00);
}

auto pcinfo_ui::get_char_name() -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x04);
}

auto pcinfo_ui::get_guild_name() -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x20);
}

auto pcinfo_ui::get_level() -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x3C);
}

auto pcinfo_ui::get_style() -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x3D);
}

auto pcinfo_ui::get_exp() -> std::uint64_t {
  return ext_client::off::field_at<std::uint64_t>(this, 0x40);
}

auto pcinfo_ui::get_strength() -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x48);
}

auto pcinfo_ui::get_intelligence() -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x4A);
}

auto pcinfo_ui::get_stat_points() -> std::uint16_t {
  return ext_client::off::field_at<std::uint16_t>(this, 0x4C);
}

auto pcinfo_ui::get_hp() -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x50);
}

auto pcinfo_ui::get_mp() -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x54);
}

auto pcinfo_ui::get_deleting() -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x58);
}

auto pcinfo_ui::get_deletion_time() -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x5C);
}

auto pcinfo_ui::get_style2() -> std::uint8_t {
  return ext_client::off::field_at<std::uint8_t>(this, 0x60);
}

auto pcinfo_ui::get_guild_name2() -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x64);
}

auto pcinfo_ui::get_avatar_ids() -> ext_client::msvc9::vector<std::uint32_t> {
  return ext_client::off::field_at<ext_client::msvc9::n_vector<std::uint32_t>>(this, 0xEC);
}

auto pcinfo_ui::get_packed_time() -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0xFC);
}

auto pcinfo_ui::get_blend_var1() -> blend_variable<std::uint8_t> {
  return blend_variable<std::uint8_t>{};
}

auto pcinfo_ui::get_blend_var2() -> blend_variable<std::uint8_t> {
  return blend_variable<std::uint8_t>{};
}

auto pcinfo_ui::get_font_texture() -> cg_font_texture {
  return cg_font_texture{};
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

auto pcinfo_ui::set_level(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x3C) = val;
}

auto pcinfo_ui::set_style(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x3D) = val;
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

auto pcinfo_ui::set_deleting(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x58) = val;
}

auto pcinfo_ui::set_deletion_time(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x5C) = val;
}

auto pcinfo_ui::set_style2(std::uint8_t val) -> void {
  ext_client::off::field_at<std::uint8_t>(this, 0x60) = val;
}

auto pcinfo_ui::set_guild_name2(ext_client::msvc9::wstring val) -> void {
  ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x64) = val;
}

auto pcinfo_ui::set_avatar_ids(ext_client::msvc9::vector<std::uint32_t> val) -> void {
  ext_client::off::field_at<ext_client::msvc9::n_vector<std::uint32_t>>(this, 0xEC) = val;
}

auto pcinfo_ui::set_packed_time(std::uint32_t val) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0xFC) = val;
}

auto pcinfo_ui::set_blend_var1(blend_variable<std::uint8_t> val) -> void {
  // TODO: implement when blend_variable type is properly defined
}

auto pcinfo_ui::set_blend_var2(blend_variable<std::uint8_t> val) -> void {
  // TODO: implement when blend_variable type is properly defined
}

auto pcinfo_ui::set_font_texture(cg_font_texture val) -> void {
  // TODO: implement when cg_font_texture type is properly defined
}
