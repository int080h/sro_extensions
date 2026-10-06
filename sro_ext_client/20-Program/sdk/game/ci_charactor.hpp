#pragma once

#include "sdk/game/ci_gid_object.hpp"
#include "sdk/types/s_position.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/vectorf.hpp"

#include <cstddef>
#include <cstdint>

class ccompound_obj;

// ---------------------------------------------------------------------------
// CICharactor — Base game character entity (Player, Monster, NPC)
// Size: 0x8C0 (2240 bytes) | Extends CIGidObject
// ---------------------------------------------------------------------------
class ci_charactor : public ci_gid_object {
public:
  static constexpr std::size_t k_class_size = 0x8C0;

  // 1. Entity Identity & Position
  auto get_unique_id() const -> std::uint32_t;
  auto get_display_name() const -> const wchar_t*;
  auto get_position() const -> const s_position*;
  auto get_compound_obj() -> ccompound_obj*;

  // 2. Vitals & Combat Stats
  auto get_hp() const -> std::uint32_t;
  auto get_mp() const -> std::uint32_t;
  auto get_max_hp() const -> std::uint32_t;
  auto get_max_mp() const -> std::uint32_t;
  auto get_display_hp() const -> std::uint32_t;

  auto set_hp(std::uint32_t val) -> void;
  auto set_mp(std::uint32_t val) -> void;
  auto set_max_hp(std::uint32_t val) -> void;
  auto set_max_mp(std::uint32_t val) -> void;

  // 3. State & Timers
  auto get_state_619() const -> std::uint8_t;
  auto get_idle_timer() const -> int;
  auto get_state_mask() const -> std::uint16_t;

  auto set_state_619(std::uint8_t val) -> void;
  auto set_idle_timer(int val) -> void;
  auto set_state_mask(std::uint16_t val) -> void;

  // 4. Actions & Emotes
  auto play_emote(unsigned char action_type, int emote_id) -> bool;

  // 5. Classification & Rarity
  auto get_refobj_id() const -> std::uint32_t;
  auto get_level() const -> std::uint32_t;
  auto is_player() const -> bool;
  auto is_monster() const -> bool;
  auto is_npc() const -> bool;
  auto is_pet() const -> bool;
  auto is_fellow_pet() const -> bool;
  auto is_growth_pet() const -> bool;
  auto is_ability_pet() const -> bool;
  auto is_ride_pet() const -> bool;
  auto get_cos_id() const -> std::uint32_t;
  auto get_owner_unique_id() const -> std::uint32_t;
  auto get_pet_archetype() const -> std::uint8_t;
  auto pet_archetype_name() const -> const char*;
  auto pet_archetype_name_loc() const -> std::string;
  auto is_guard() const -> bool;
  auto is_teleport() const -> bool;
  auto get_rarity() const -> std::uint8_t;
  auto rarity_name() const -> const char*;
  auto rarity_name_loc() const -> std::string;
  auto is_party_mob() const -> bool;
  auto is_unique() const -> bool;
  auto is_champion() const -> bool;
  auto is_giant() const -> bool;

  // 6. Overhead & Height Dimensions
  auto get_model_height() const -> float;
  auto get_overhead_3d_position() const -> vector3f;
  auto get_engine_projected_overhead(float& out_screen_x, float& out_screen_y) const -> bool;
};

