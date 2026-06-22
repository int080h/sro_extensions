#pragma once

#include "cobj.hpp"
#include "utils/offsets.hpp"

class ccompoundobj;

// SEquipmentSlot: slot structure for items/equipment
struct SEquipmentSlot {
  std::uint32_t item_id;
  std::uint8_t pad[16];
};

// SActionSlot: slot structure for emotes and actions
struct SActionSlot {
  std::uint8_t action_type;
  std::uint8_t pad[7];
  std::uint32_t action_id;
  std::uint8_t pad2[8];
};

// si_char_info: Plain structure base for characters
class si_char_info {
  // Plain base class with no virtual methods
};

// ci_charactor: character game-object base (monster, NPC, player, etc.)
// Size 0x8C0 (2240 bytes).
class ci_charactor : public ci_gid_object, public si_char_info {
public:
  union {
    DEFINE_MEMBER_0(std::uint8_t m_class_link[20], "class_link");
    DEFINE_MEMBER_N(std::uint32_t m_unknown_38c, 0x14);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_390, 0x18);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_398, 0x20);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_39c, 0x24);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3a0, 0x28);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3a4, 0x2C);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3a8, 0x30);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3ac, 0x34);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3ad, 0x35);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3b0, 0x38);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3b4, 0x3C);
    DEFINE_MEMBER_N(void* m_item_map, 0x44);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3c0, 0x48);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3c4, 0x4C);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3c5, 0x4D);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3c6, 0x4E);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3c7, 0x4F);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3c8, 0x50);
    DEFINE_MEMBER_N(void* m_skill_map, 0x58);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3d4, 0x5C);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3dc, 0x64);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3e0, 0x68);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_3e4, 0x6C);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_3e8, 0x70);
    DEFINE_MEMBER_N(float m_unknown_454, 0xDC);
    DEFINE_MEMBER_N(float m_unknown_458, 0xE0);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_46c, 0xF4);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_544, 0x1CC);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_545, 0x1CD);
    DEFINE_MEMBER_N(std::uint8_t m_action_lock, 0x1D4);
    DEFINE_MEMBER_N(float m_unknown_548, 0x1D0);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_54c, 0x1D4);
    DEFINE_MEMBER_N(float m_unknown_550, 0x1D8);
    DEFINE_MEMBER_N(std::uint32_t m_hp, 0x1DC);
    DEFINE_MEMBER_N(std::uint32_t m_mp, 0x1E0);
    DEFINE_MEMBER_N(std::uint32_t m_max_hp, 0x1E4);
    DEFINE_MEMBER_N(std::uint32_t m_max_mp, 0x1E8);
    DEFINE_MEMBER_N(std::uint32_t m_display_hp, 0x1EC);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_568, 0x1F0);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_56c, 0x1F4);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_570, 0x1F8);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_574, 0x1FC);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_575, 0x1FD);
    DEFINE_MEMBER_N(ext_client::msvc9::wstring m_guild_name, 0x200);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_5ec, 0x274);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_5f0, 0x278);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_608, 0x290);
    DEFINE_MEMBER_N(void* m_cooldowns, 0x298);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_614, 0x29C);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_618, 0x2A0);
    DEFINE_MEMBER_N(std::uint8_t m_state_619, 0x2A1);
    DEFINE_MEMBER_N(std::uint8_t m_unknown_61a, 0x2A2);
    DEFINE_MEMBER_N(int m_idle_timer, 0x2A4);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_620, 0x2A8);
    DEFINE_MEMBER_N(std::uint32_t m_unknown_624, 0x2AC);
    DEFINE_MEMBER_N(SActionSlot m_action_slots[16], 0x2B0);
    DEFINE_MEMBER_N(SEquipmentSlot m_equipment_slots[15], 0x2C0);
    DEFINE_MEMBER_N(std::uint32_t m_last_equip_slot_item_id, 0x3EC);
    DEFINE_MEMBER_N(std::uint16_t m_state_mask, 0x3F0);
    DEFINE_MEMBER_N(std::uint8_t m_sub_774[332], 0x3FC);
    DEFINE_MEMBER_N(void* m_target_ptr, 0x480);
    DEFINE_MEMBER_N(std::uint32_t m_action_state, 0x518);
  };
public:
  ci_charactor() {}
  ~ci_charactor() override {}

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

};


