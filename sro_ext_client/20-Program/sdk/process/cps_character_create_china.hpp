#pragma once

#include "sdk/process/cps_outer_interface.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSCharacterCreateChina — Character creation process (China region)
// Native VTable: 0x0103064C (41 slots) | Class Size: 0x374 | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_character_create_china : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x0103064C;
  static constexpr std::size_t   k_class_size  = 0x0374;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_character_create_china*;
  static auto create() -> cps_character_create_china*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_character_create_china*;

  // 2. Character Slots & Appearance
  auto get_appearance_list() -> ext_client::msvc9::n_vector<void*>&;
  auto get_max_char_count() const -> int;
  auto get_cur_char_count() const -> int;
};
