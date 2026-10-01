#pragma once

#include "sdk/game/ci_charactor.hpp"
#include "utils/msvc9_stl.hpp"

// cic_user: user character wrapper class (base of cic_player and cic_script_obj)
// Size 2544 (0x9F0) bytes.
class cic_user : public ci_charactor {
public:
  auto get_display_name() -> ext_client::msvc9::wstring;
  auto get_equipped_weapon() -> void*;
  auto get_user_guild_name() -> ext_client::msvc9::wstring;
  auto set_display_name(ext_client::msvc9::wstring val) -> void;
  auto set_equipped_weapon(void* val) -> void;
  auto set_user_guild_name(ext_client::msvc9::wstring val) -> void;
public:
  cic_user() {}
  ~cic_user() override {}
};
