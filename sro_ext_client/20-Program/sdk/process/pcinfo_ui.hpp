#pragma once

#include "sdk/types/blend_variable.hpp"
#include "sdk/ui/cg_font_texture.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// pcinfo_ui — Character slot info record in character selection (CPSCharacterSelect)
// Size: 0x1D8 (472 bytes) | Stored in 0x8C0 array entries
// Holds character identity, appearance, stats, deletion timers, and packed logout time.
// ---------------------------------------------------------------------------
struct pcinfo_ui {
  static constexpr std::size_t k_class_size = 0x1D8;

  // 1. Identity & Guild (Getters / Setters)
  auto get_model_id() const -> std::uint32_t;
  auto get_char_name() const -> ext_client::msvc9::wstring;
  auto get_guild_name() const -> ext_client::msvc9::wstring;
  auto get_guild_name2() const -> ext_client::msvc9::wstring;

  auto set_model_id(std::uint32_t val) -> void;
  auto set_char_name(ext_client::msvc9::wstring val) -> void;
  auto set_guild_name(ext_client::msvc9::wstring val) -> void;
  auto set_guild_name2(ext_client::msvc9::wstring val) -> void;

  // 2. Character Progression & Stats (Getters / Setters)
  auto get_level() const -> std::uint8_t;
  auto get_style() const -> std::uint8_t;
  auto get_style2() const -> std::uint8_t;
  auto get_exp() const -> std::uint64_t;
  auto get_strength() const -> std::uint16_t;
  auto get_intelligence() const -> std::uint16_t;
  auto get_stat_points() const -> std::uint16_t;
  auto get_hp() const -> std::uint32_t;
  auto get_mp() const -> std::uint32_t;

  auto set_level(std::uint8_t val) -> void;
  auto set_style(std::uint8_t val) -> void;
  auto set_style2(std::uint8_t val) -> void;
  auto set_exp(std::uint64_t val) -> void;
  auto set_strength(std::uint16_t val) -> void;
  auto set_intelligence(std::uint16_t val) -> void;
  auto set_stat_points(std::uint16_t val) -> void;
  auto set_hp(std::uint32_t val) -> void;
  auto set_mp(std::uint32_t val) -> void;

  // 3. Timers & Deletion State
  auto get_deleting() const -> std::uint8_t;
  auto get_deletion_time() const -> std::uint32_t;
  auto get_packed_time() const -> std::uint32_t;

  auto set_deleting(std::uint8_t val) -> void;
  auto set_deletion_time(std::uint32_t val) -> void;
  auto set_packed_time(std::uint32_t val) -> void;

  // 4. Equipment & Visual Properties
  auto get_avatar_ids() const -> ext_client::msvc9::vector<std::uint32_t>;
  auto get_blend_var1() const -> blend_variable<std::uint8_t>;
  auto get_blend_var2() const -> blend_variable<std::uint8_t>;
  auto get_font_texture() const -> cg_font_texture;

  auto set_avatar_ids(ext_client::msvc9::vector<std::uint32_t> val) -> void;
  auto set_blend_var1(blend_variable<std::uint8_t> val) -> void;
  auto set_blend_var2(blend_variable<std::uint8_t> val) -> void;
  auto set_font_texture(cg_font_texture val) -> void;
};
