#pragma once

#include "sdk/process/cps_outer_interface.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CPSCharacterCreateEurope — Character creation process (Europe region)
// Native VTable: 0x01031C74 (41 slots) | Class Size: 0x378 | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_character_create_europe : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01031C74;
  static constexpr std::size_t   k_class_size  = 0x0378;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_character_create_europe*;
  static auto create() -> cps_character_create_europe*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_character_create_europe*;

  // 2. Character Slots & Appearance
  auto get_appearance_list() -> ext_client::msvc9::n_vector<void*>&;
  auto get_max_char_count() const -> int;
  auto get_cur_char_count() const -> int;
};
