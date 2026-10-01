#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstdint>

class cif_static;

// CPSVersionCheck: gateway connect, version verify, loading splash, textdata preload.
// vt @ 0x10349CC (41 slots). Size 0x118.
class cps_version_check : public cps_outer_interface {
public:
  static constexpr std::size_t vtable_slots = 41;

  auto is_data_load_started() const -> bool;
  auto set_data_load_started(bool value) -> void;
  auto find_loading_banner_widget() -> cif_static*;

  static auto create() -> cps_version_check*;
  static auto current() -> cps_version_check*;
  static auto is_active() -> bool;
  static auto version_error_code() -> int;
  static auto version_error_tag() -> int;
  static auto connect_gateway() -> bool;
  static auto load_game_textdata(void* cg_interface) -> bool;
  static auto set_version_active(bool active) -> void;
};
