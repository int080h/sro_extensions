#pragma once

#include "sdk/process/cps_outer_interface.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

// CPSCharacterCreateChina: character creation screen — China region (resinfo\pscharactercreatechina.txt).
// vt @ 0x103064C (41 slots). Size 0x374.
class cps_character_create_china : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto get_appearance_list() -> ext_client::msvc9::n_vector<void*>&;
  auto get_max_char_count() const -> int;
  auto get_cur_char_count() const -> int;

  static auto create() -> cps_character_create_china*;
  static auto current() -> cps_character_create_china*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_character_create_china*;
};
