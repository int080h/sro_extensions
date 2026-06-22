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

private:
  union {
    DEFINE_MEMBER_0(std::uint8_t m_data_load_started, "data_load_started");
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cps_version_check::size - sizeof(cps_outer_interface)], "pad_end");
  };

  static inline auto check_layout() -> void {
    
  }
};
