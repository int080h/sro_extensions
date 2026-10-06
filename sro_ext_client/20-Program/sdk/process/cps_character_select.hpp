#pragma once

#include "sdk/process/cps_outer_interface.hpp"
#include "sdk/game/cic_deco_character.hpp"
#include "sdk/process/pcinfo_ui.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSCharacterSelect — Character selection screen process (resinfo\pscharacterselect.txt)
// Native VTable: 0x0103298C (41 slots) | Class Size: 0x240
// Manages character slots, 3D character preview models, and enter-world fade transitions.
// ---------------------------------------------------------------------------
class cps_character_select : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x0103298C;
  static constexpr std::size_t   k_class_size  = 0x0240;
  static constexpr std::size_t   vtable_slots  = 41;

  // SCharacterInfo in sro_client.exe: total size 2712 (0xA98) bytes.
  // Offset 0x000: CICDecoCharacter (size 0x8C0 / 2240 bytes)
  // Offset 0x8C0: pcinfo_ui (size 0x1D8 / 472 bytes)
  struct s_character_info {
    std::uint8_t deco_character[0x8C0];
    pcinfo_ui ui_info;
  };

  // 1. Type Inspection & Process Resolution
  static auto get_current() -> cps_character_select*;
  static auto get_resolve_live() -> cps_character_select*;
  static auto is_live(const void* ptr) -> bool;
  static auto create() -> cps_character_select*;

  // 2. Character Slot Queries & 3D Previews
  auto get_char_count() const -> int;
  auto get_selected_slot() const -> int;
  static auto get_selected_slot_index() -> int;
  auto get_character_at(int index) const -> pcinfo_ui*;
  auto get_deco_character_at(int index) const -> cic_deco_character*;
  auto get_characters() -> ext_client::msvc9::n_vector<s_character_info*>&;
  auto get_char_objects() -> ext_client::msvc9::n_vector<void*>&;

  // 3. State & UI Properties
  auto get_page_index() const -> int;
  auto get_page_count() const -> int;
  auto get_ui_map() -> ext_client::msvc9::n_map<int, void*>&;
  static auto is_pin_required() -> bool;
  auto set_login_phase(int phase) -> void;

  // 4. Native Actions & Lifecycle
  auto begin_enter_world_fade() -> int;
  static auto request_character_list() -> void;
  auto show_delete_dialog(bool show) -> int;
  auto handle_character_list(void* packet) -> int;
  auto init_defaults() -> int;
};
