#pragma once

#include "sdk/game/cic_user.hpp"
#include "sdk/game/c_state_deliver.hpp"

#include <cstdint>

// cic_player: player game-object instance
// Size 0x3A94 (15000 bytes).
class cic_player : public cic_user, public c_state_deliver {
public:
  auto get_level() -> std::uint8_t;
  auto get_exp() -> std::uint64_t;
  auto get_sp() -> std::uint32_t;
  auto get_strength() -> std::uint16_t;
  auto get_intelligence() -> std::uint16_t;
  auto get_stat_points() -> std::uint32_t;
  auto get_gamemaster() -> std::uint32_t;
  auto set_level(std::uint8_t val) -> void;
  auto set_exp(std::uint64_t val) -> void;
  auto set_sp(std::uint32_t val) -> void;
  auto set_strength(std::uint16_t val) -> void;
  auto set_intelligence(std::uint16_t val) -> void;
  auto set_stat_points(std::uint32_t val) -> void;
  auto set_gamemaster(std::uint32_t val) -> void;
public:
  cic_player() {}
  ~cic_player() override {}

  static auto local() -> cic_player*;
  auto name() const -> const wchar_t*;
  auto guild_name() const -> const wchar_t*;
  auto is_gamemaster() const -> bool;
  auto level() const -> std::uint8_t;
  auto exp() const -> std::uint64_t;
  auto sp() const -> std::uint32_t;
  auto gold() const -> std::uint64_t;
  auto strength() const -> std::uint16_t;
  auto intelligence() const -> std::uint16_t;
  auto attribute_points() const -> std::uint32_t;
};
