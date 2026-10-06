#pragma once

#include "sdk/ui/cif_static.hpp"

// ---------------------------------------------------------------------------
// CIFGauge — Progress / HP / MP gauge widget (inherits CIFStatic)
// Native VTable: 0x00FF056C | Class Size: 0x3A4
// ---------------------------------------------------------------------------
class cif_gauge : public cif_static {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x00FF056C;
  static constexpr std::size_t   k_class_size  = 0x03A4;

  // 1. Ratio & Speed Queries
  auto get_current_ratio() const -> float;
  auto get_target_ratio() const -> float;
  auto get_speed() const -> float;
  auto get_current_percent() const -> int;

  // 2. Type Queries
  static auto is_gauge(const void* wnd) -> bool;
};
