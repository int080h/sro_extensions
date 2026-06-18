#pragma once

#include "cicharactor.hpp"

// cic_user: user character wrapper class (base of cic_player and cic_script_obj)
// Size 2544 (0x9F0) bytes.
class cic_user : public ci_charactor {
public:
  PAD_TO(sizeof(ci_charactor), 2544);
};
static_assert(sizeof(cic_user) == 2544, "cic_user size mismatch");

// c_state_deliver: State delivery base class with virtual destructor
class c_state_deliver {
public:
  virtual ~c_state_deliver() = default;
};

// cic_player: player game-object instance
// Size 0x3A94 (15000 bytes).
class cic_player : public cic_user, public c_state_deliver {
public:
  // Get local player instance
  static auto local() -> cic_player*;

  // Get character name
  auto name() const -> const wchar_t*;

  // Get guild name
  auto guild_name() const -> const wchar_t*;

  // Check if character is GameMaster
  auto is_gamemaster() const -> bool;

  // Attributes
  auto level() const -> std::uint8_t;
  auto exp() const -> std::uint64_t;
  auto sp() const -> std::uint32_t;
  auto gold() const -> std::uint64_t;
  auto strength() const -> std::uint16_t;
  auto intelligence() const -> std::uint16_t;
  auto attribute_points() const -> std::uint32_t;

private:
  PAD_TO(sizeof(cic_user) + sizeof(c_state_deliver), 0x3A94);
};

static_assert(sizeof(cic_player) == 0x3A94, "cic_player size mismatch");


