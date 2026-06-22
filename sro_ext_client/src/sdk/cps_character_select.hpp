#pragma once

#include "cps_outer_interface.hpp"
#include "s_character_info.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

// CPSCharacterSelect vtable (0x103298C, 41 methods) — same slot layout as CPSOuterInterface.
using cps_character_select_vtable = cps_outer_interface_vtable;

// CPSCharacterSelect: character list, 3D previews, enter-world fade (resinfo\pscharacterselect.txt). Size 0x240.
class cps_character_select : public cps_outer_interface {
public:
  DECLARE_SDK_VTABLE(cps_character_select_vtable, character_select_vftable)

  struct s_character_info : public cic_deco_character, public pcinfo_ui {
  };

  static auto create() -> cps_character_select*;
  static auto current() -> cps_character_select*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_character_select*;
  static auto selected_slot_index() -> int;
  static auto pin_required() -> bool;
  static auto request_character_list() -> void;

  auto char_count() const -> int;
  auto page_index() const -> int;
  auto page_count() const -> int;
  auto selected_slot() const -> int;
  auto character_at(int index) const -> pcinfo_ui*;

  auto handle_character_list(void* packet) -> int;
  auto begin_enter_world_fade() -> int;
  auto show_delete_dialog(bool show) -> int;
  auto init_defaults() -> int;
  auto set_login_phase(int phase) -> void;


private:
  union {
    DEFINE_MEMBER_0(void* m_secondary_vftable, "secondary_vftable");
    DEFINE_MEMBER_N(int m_anim_state, 0x0C);
    DEFINE_MEMBER_N(std::uint8_t m_list_refresh_flag, 0x10);
    DEFINE_MEMBER_N(std::uint8_t m_delete_dialog_mode, 0x11);
    DEFINE_MEMBER_N(std::uint8_t m_char_count, 0x20);
    DEFINE_MEMBER_N(std::uint8_t m_page_index, 0x21);
    DEFINE_MEMBER_N(std::uint8_t m_page_count, 0x22);
    DEFINE_MEMBER_N(std::uint8_t m_logout_msg_pending, 0x23);
    DEFINE_MEMBER_N(std::uint8_t m_selected_slot, 0x24);
    DEFINE_MEMBER_N(void* m_characters_begin, 0x2C);
    DEFINE_MEMBER_N(void* m_characters_end, 0x30);
    DEFINE_MEMBER_N(void* m_characters_capacity, 0x34);
    DEFINE_MEMBER_N(void* m_rename_wstring, 0x38);
    DEFINE_MEMBER_N(void* m_char_objects_begin, 0x3C);
    DEFINE_MEMBER_N(void* m_char_objects_end, 0x40);
    DEFINE_MEMBER_N(void* m_char_objects_capacity, 0x44);
    DEFINE_MEMBER_N(int m_ui_map_key, 0x48);
    DEFINE_MEMBER_N(void* m_ui_map_tree, 0x4C);
    DEFINE_MEMBER_N(int m_ui_map_count, 0x50);
    DEFINE_MEMBER_N(int m_unity_mode, 0x54);
    DEFINE_MEMBER_N(std::uint8_t m_slot_rig_0[ext_client::offsets::cps_character_select::slot_rig::size], 0x58);
    DEFINE_MEMBER_N(std::uint8_t m_slot_rig_1[ext_client::offsets::cps_character_select::slot_rig::size], 0x88);
    DEFINE_MEMBER_N(std::uint8_t m_slot_rig_2[ext_client::offsets::cps_character_select::slot_rig::size], 0xB8);
    DEFINE_MEMBER_N(std::uint8_t m_slot_rig_extra[ext_client::offsets::cps_character_select::slot_rig::size], 0xE8);
    DEFINE_MEMBER_N(int m_field_228, 0x11C);
    DEFINE_MEMBER_N(int m_field_564, 0x128);
    DEFINE_MEMBER_N(std::uint8_t m_field_568, 0x12C);
    DEFINE_MEMBER_N(std::uint8_t m_sec_pw_flag_a, 0x12D);
    DEFINE_MEMBER_N(std::uint8_t m_sec_pw_flag_b, 0x12E);
    DEFINE_MEMBER_N(std::uint8_t m_autologin_checked, 0x12F);
    DEFINE_MEMBER_N(float m_fade_value, 0x130);
  };
};


