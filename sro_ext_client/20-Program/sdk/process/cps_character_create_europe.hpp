#pragma once

#include "sdk/process/cps_outer_interface.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

// CPSCharacterCreateEurope: character creation screen — Europe region (resinfo\pscharactercreateeurope.txt).
// vt @ 0x1031C74 (41 slots). Size 0x378.
class cps_character_create_europe : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto get_appearance_list() -> ext_client::msvc9::n_vector<void*>&;
  auto get_max_char_count() const -> int;
  auto get_cur_char_count() const -> int;

  static auto create() -> cps_character_create_europe*;
  static auto current() -> cps_character_create_europe*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_character_create_europe*;
};
