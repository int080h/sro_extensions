#pragma once

#include "cps_outer_interface.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

// CPSVersionCheck vtable — same slot layout as CPSOuterInterface.
using cps_version_check_vtable = cps_outer_interface_vtable;

// CPSVersionCheck: gateway connect, version verify, loading splash, textdata preload.
class cps_version_check : public cps_outer_interface {
public:
  static auto create() -> cps_version_check*;
  static auto current() -> cps_version_check*;
  static auto is_active() -> bool;
  static auto version_error_code() -> int;
  static auto version_error_tag() -> int;
  static auto connect_gateway() -> bool;
  static auto load_game_textdata(void* cg_interface) -> bool;
  static auto set_version_active(bool active) -> void;

  auto data_load_started() const -> bool;
  auto set_data_load_started(bool value) -> void;

  static void set_current(cps_version_check* instance);

private:
  std::uint8_t m_data_load_started;
  PAD_TO(ext_client::offsets::cps_version_check::fields::data_load_started + sizeof(std::uint8_t),
         ext_client::offsets::cps_version_check::size);

  static inline auto check_layout() -> void {
    static_assert(sizeof(cps_version_check) == ext_client::offsets::cps_version_check::size, "cps_version_check size mismatch");
  }
};
