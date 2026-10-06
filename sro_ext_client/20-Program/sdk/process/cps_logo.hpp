#pragma once

#include "sdk/process/cps_outer_interface.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CPSLogo — Logo splash screen process (resinfo\pslogo.txt)
// Native VTable: 0x01032A9C (41 slots) | Class Size: 0x118 | Extends CPSOuterInterface
// ---------------------------------------------------------------------------
class cps_logo : public cps_outer_interface {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01032A9C;
  static constexpr std::size_t   k_class_size  = 0x0118;
  static constexpr std::size_t   vtable_slots  = 41;

  // 1. Type Inspection & Resolution
  static auto current() -> cps_logo*;
  static auto create() -> cps_logo*;
  static auto is_live(const void* ptr) -> bool;
  static auto resolve_live() -> cps_logo*;
};
