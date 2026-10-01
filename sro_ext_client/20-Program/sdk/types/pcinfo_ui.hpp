#pragma once

#include "sdk/types/blend_variable.hpp"
#include "sdk/ui/cg_font_texture.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

struct pcinfo_ui {
  pcinfo_ui() {}
  ~pcinfo_ui() {}
  auto get_model_id() -> std::uint32_t;
  auto get_char_name() -> ext_client::msvc9::wstring;
  auto get_guild_name() -> ext_client::msvc9::wstring;
  auto get_level() -> std::uint8_t;
  auto get_style() -> std::uint8_t;
  auto get_exp() -> std::uint64_t;
  auto get_strength() -> std::uint16_t;
  auto get_intelligence() -> std::uint16_t;
  auto get_stat_points() -> std::uint16_t;
  auto get_hp() -> std::uint32_t;
  auto get_mp() -> std::uint32_t;
  auto get_deleting() -> std::uint8_t;
  auto get_deletion_time() -> std::uint32_t;
  auto get_style2() -> std::uint8_t;
  auto get_guild_name2() -> ext_client::msvc9::wstring;
  auto get_avatar_ids() -> ext_client::msvc9::vector<std::uint32_t>;
  auto get_packed_time() -> std::uint32_t;
  auto get_blend_var1() -> blend_variable<std::uint8_t>;
  auto get_blend_var2() -> blend_variable<std::uint8_t>;
  auto get_font_texture() -> cg_font_texture;
  auto set_model_id(std::uint32_t val) -> void;
  auto set_char_name(ext_client::msvc9::wstring val) -> void;
  auto set_guild_name(ext_client::msvc9::wstring val) -> void;
  auto set_level(std::uint8_t val) -> void;
  auto set_style(std::uint8_t val) -> void;
  auto set_exp(std::uint64_t val) -> void;
  auto set_strength(std::uint16_t val) -> void;
  auto set_intelligence(std::uint16_t val) -> void;
  auto set_stat_points(std::uint16_t val) -> void;
  auto set_hp(std::uint32_t val) -> void;
  auto set_mp(std::uint32_t val) -> void;
  auto set_deleting(std::uint8_t val) -> void;
  auto set_deletion_time(std::uint32_t val) -> void;
  auto set_style2(std::uint8_t val) -> void;
  auto set_guild_name2(ext_client::msvc9::wstring val) -> void;
  auto set_avatar_ids(ext_client::msvc9::vector<std::uint32_t> val) -> void;
  auto set_packed_time(std::uint32_t val) -> void;
  auto set_blend_var1(blend_variable<std::uint8_t> val) -> void;
  auto set_blend_var2(blend_variable<std::uint8_t> val) -> void;
  auto set_font_texture(cg_font_texture val) -> void;
};
