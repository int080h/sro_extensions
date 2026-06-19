#pragma once

#include "cobj.hpp"
#include "utils/offsets.hpp"

// Forward declaration of ccompoundobj
class ccompoundobj;

// SPosition: 3D coordinates in Silkroad's regional space
struct SPosition {
  std::uint16_t region_id;
  float x;
  float z;
  float y;

  auto region_x() const -> std::uint8_t;
  auto region_y() const -> std::uint8_t;
};

// si_char_info: Plain structure base for characters
class si_char_info {
  // Plain base class with no virtual methods
};

// ci_charactor: character game-object base (monster, NPC, player, etc.)
// Size 0x8C0 (2240 bytes).
class ci_charactor : public ci_gid_object, public si_char_info {
public:
  // Play emote action on character entity using the game's internal emote function
  auto play_emote(unsigned char action_type, int emote_id) -> bool;

  // Get the compound 3D model object (ccompoundobj*)
  auto get_compound_obj() -> ccompoundobj*;

  // Get character position / coordinates
  auto position() const -> const SPosition*;

  // Get stats
  auto hp() const -> std::uint32_t;
  auto display_hp() const -> std::uint32_t;
  auto mp() const -> std::uint32_t;
  auto max_hp() const -> std::uint32_t;
  auto max_mp() const -> std::uint32_t;

  // Set stats (invokes virtual functions)
  auto set_hp(std::uint32_t val) -> void;
  auto set_mp(std::uint32_t val) -> void;
  auto set_max_hp(std::uint32_t val) -> void;
  auto set_max_mp(std::uint32_t val) -> void;

  // Emote state / status flags
  auto state_619() const -> std::uint8_t;
  auto set_state_619(std::uint8_t val) -> void;

  // Idle timer (in milliseconds)
  auto idle_timer() const -> int;
  auto set_idle_timer(int val) -> void;

  // State mask (16-bit word) at offset 0x768
  auto state_mask() const -> std::uint16_t;
  auto set_state_mask(std::uint16_t val) -> void;

  // Packet / entity-manager unique id (+0x34, sub_9D5C20)
  auto unique_id() const -> std::uint32_t;

  // Player charname buffer (+0xD0) or mob refdata name
  auto display_name() const -> const wchar_t*;

private:
  PAD_TO(sizeof(ci_gid_object) + sizeof(si_char_info), 0x8C0);
};

static_assert(sizeof(ci_charactor) == 0x8C0, "ci_charactor size mismatch");


