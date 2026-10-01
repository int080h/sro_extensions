#pragma once

#include "sdk/game/ci_gid_object.hpp"
#include "sdk/types/s_equipment_slot.hpp"
#include "sdk/types/s_action_slot.hpp"
#include "sdk/types/s_position.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

class ccompound_obj;

// ci_charactor: character game-object base (monster, NPC, player, etc.)
// Size 0x8C0 (2240 bytes).
class ci_charactor : public ci_gid_object {
public:
  auto get_item_map() -> void*;
  auto get_skill_map() -> void*;
  auto get_action_lock() -> std::uint8_t;
  auto get_hp() const -> std::uint32_t;
  auto get_mp() const -> std::uint32_t;
  auto get_max_hp() const -> std::uint32_t;
  auto get_max_mp() const -> std::uint32_t;
  auto get_display_hp() const -> std::uint32_t;
  auto get_guild_name() -> ext_client::msvc9::wstring;
  auto get_cooldowns() -> void*;
  auto get_state_619() const -> std::uint8_t;
  auto get_idle_timer() const -> int;
  auto get_last_equip_slot_item_id() -> std::uint32_t;
  auto get_state_mask() const -> std::uint16_t;
  auto get_target_ptr() -> void*;
  auto get_action_state() -> std::uint32_t;
  auto get_compound_obj() -> ccompound_obj*;
  auto get_position() const -> const s_position*;
  auto get_unique_id() const -> std::uint32_t;
  auto get_display_name() const -> const wchar_t*;

  auto set_item_map(void* val) -> void;
  auto set_skill_map(void* val) -> void;
  auto set_action_lock(std::uint8_t val) -> void;
  auto set_hp(std::uint32_t val) -> void;
  auto set_mp(std::uint32_t val) -> void;
  auto set_max_hp(std::uint32_t val) -> void;
  auto set_max_mp(std::uint32_t val) -> void;
  auto set_display_hp(std::uint32_t val) -> void;
  auto set_guild_name(ext_client::msvc9::wstring val) -> void;
  auto set_cooldowns(void* val) -> void;
  auto set_state_619(std::uint8_t val) -> void;
  auto set_idle_timer(int val) -> void;
  auto set_last_equip_slot_item_id(std::uint32_t val) -> void;
  auto set_state_mask(std::uint16_t val) -> void;
  auto set_target_ptr(void* val) -> void;
  auto set_action_state(std::uint32_t val) -> void;

  ci_charactor() {}
  ~ci_charactor() override {}

  auto play_emote(unsigned char action_type, int emote_id) -> bool;
};
