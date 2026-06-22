#pragma once

#include "cicharactor.hpp"

// cic_user: user character wrapper class (base of cic_player and cic_script_obj)
// Size 2544 (0x9F0) bytes.
class cic_user : public ci_charactor {
public:
  union {
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_display_name, 0x0C);
    DEFINE_MEMBER_N(void* m_equipped_weapon, 0x34);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_user_guild_name, 0x3C);
  };
public:
  cic_user() {}
  ~cic_user() override {}
};

// c_state_deliver: State delivery base class with virtual destructor
class c_state_deliver {
public:
  virtual ~c_state_deliver() = default;
};

// cic_player: player game-object instance
// Size 0x3A94 (15000 bytes).
class cic_player : public cic_user, public c_state_deliver {
public:
  union {
    DEFINE_MEMBER_N(std::uint16_t m_unknown_9f8, 0x04);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_a08, 0x14);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_a0c, 0x18);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_a10, 0x1C);
    DEFINE_MEMBER_N(std::uint8_t m_level, 0x20);
    DEFINE_MEMBER_N(std::uint64_t m_exp, 0x24);
    DEFINE_MEMBER_N(std::uint32_t m_sp, 0x2C);
    DEFINE_MEMBER_N(std::uint16_t m_strength, 0x30);
    DEFINE_MEMBER_N(std::uint16_t m_intelligence, 0x32);
    DEFINE_MEMBER_N(std::uint32_t m_stat_points, 0x34);
    DEFINE_MEMBER_N(std::uint32_t m_gamemaster, 0x3064);
  };
public:
  cic_player() {}
  ~cic_player() override {}

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

};


