#pragma once

#include "sdk/ui/cif_static.hpp"

// CIFGauge — progress/HP/MP gauge widget (vtable 0x00FF056C, inherits CIFStatic).
//
// Native memory layout:
//   +0x000..+0x083  CGWnd base
//   +0x084..+0x1C3  CTextBoard (secondary MI base)
//   +0x1C4..+0x373  CIFWnd data (ui res map @ +0x1C4)
//   +0x374..+0x397  CIFStatic data
//   +0x398 (920)    float m_current_ratio (0.0f .. 1.0f)
//   +0x39C (924)    float m_target_ratio  (0.0f .. 1.0f)
//   +0x3A0 (928)    float m_speed         (lerp rate, e.g. 0.01f or 0.1f)

class cif_gauge : public cif_static {
public:
  auto get_current_ratio() const -> float;
  auto get_target_ratio() const -> float;
  auto get_speed() const -> float;

  auto get_current_percent() const -> int;

  static auto is_gauge(const void* wnd) -> bool;
};
