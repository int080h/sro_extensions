#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstddef>
#include <cstdint>

class cif_static;

// ---------------------------------------------------------------------------
// CPSVersionCheck — Gateway connect, version verification, loading splash screen
// Native VTable: 0x010349CC (41 slots) | Class Size: 0x118 | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_version_check : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x010349CC;
  static constexpr std::size_t   k_class_size  = 0x0118;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Current Instance
  static auto current() -> cps_version_check*;
  static auto create() -> cps_version_check*;
  static auto is_active() -> bool;
  static auto set_version_active(bool active) -> void;

  // 2. Loading State & Banner Widget
  auto is_data_load_started() const -> bool;
  auto set_data_load_started(bool value) -> void;
  auto find_loading_banner_widget() -> cif_static*;

  // 3. Gateway & Textdata Loading
  static auto connect_gateway() -> bool;
  static auto load_game_textdata(void* cg_interface) -> bool;
  static auto version_error_code() -> int;
  static auto version_error_tag() -> int;
};
