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
  static auto sync_current() -> cps_character_select*;
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


  static void set_current(cps_character_select* instance);

private:
  void* m_secondary_vftable;

  PAD_TO(ext_client::offsets::cps_outer_interface::derived_region_begin + sizeof(void*),
         ext_client::offsets::cps_character_select::fields::anim_state);
  int m_anim_state;
  std::uint8_t m_list_refresh_flag;
  std::uint8_t m_delete_dialog_mode;

  PAD_TO(ext_client::offsets::cps_character_select::fields::delete_dialog_mode + 1,
         ext_client::offsets::cps_character_select::fields::char_count);
  std::uint8_t m_char_count;
  std::uint8_t m_page_index;
  std::uint8_t m_page_count;
  std::uint8_t m_logout_msg_pending;
  std::uint8_t m_selected_slot;

  PAD_TO(ext_client::offsets::cps_character_select::fields::selected_slot + 1,
         ext_client::offsets::cps_character_select::fields::characters_begin);
  void* m_characters_begin;
  void* m_characters_end;
  void* m_characters_capacity;
  void* m_rename_wstring;
  void* m_char_objects_begin;
  void* m_char_objects_end;
  void* m_char_objects_capacity;
  int m_ui_map_key;
  void* m_ui_map_tree;
  int m_ui_map_count;
  int m_unity_mode;

  std::uint8_t m_slot_rig_0[ext_client::offsets::cps_character_select::slot_rig::size];
  std::uint8_t m_slot_rig_1[ext_client::offsets::cps_character_select::slot_rig::size];
  std::uint8_t m_slot_rig_2[ext_client::offsets::cps_character_select::slot_rig::size];
  std::uint8_t m_slot_rig_extra[ext_client::offsets::cps_character_select::slot_rig::size];

  PAD_TO(ext_client::offsets::cps_character_select::fields::slot_rig_extra + ext_client::offsets::cps_character_select::slot_rig::size,
         ext_client::offsets::cps_character_select::fields::field_228);
  int m_field_228;

  PAD_TO(ext_client::offsets::cps_character_select::fields::field_228 + sizeof(int),
         ext_client::offsets::cps_character_select::fields::field_564);
  int m_field_564;
  std::uint8_t m_field_568;
  std::uint8_t m_sec_pw_flag_a;
  std::uint8_t m_sec_pw_flag_b;
  std::uint8_t m_autologin_checked;
  float m_fade_value;

  static inline auto check_layout() -> void {
    static_assert(sizeof(cps_character_select) == ext_client::offsets::cps_character_select::size, "cps_character_select size mismatch");
    static_assert(sizeof(cps_character_select::s_character_info) == 0xA98, "s_character_info size mismatch");
  }
};


