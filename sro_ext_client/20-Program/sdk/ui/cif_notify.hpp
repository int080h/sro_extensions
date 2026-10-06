#pragma once

#include "sdk/ui/cif_static.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CIFNotify — On-screen notification banner (extends CIFStatic)
// ---------------------------------------------------------------------------
class cif_notify : public cif_static {
public:
  // 1. Notification State & Visibility
  auto is_active() const -> bool;
  auto set_active(bool active) -> void;

  // 2. Timing & Geometry
  auto get_duration() const -> std::uint32_t;
  auto set_duration(std::uint32_t ms) -> void;
  auto get_y_position() const -> std::int32_t;
  auto set_y_position(std::int32_t y) -> void;

  // 3. Child Controls & Content
  auto get_static_text() -> cif_static*;
  auto set_background_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> void;
  auto show_message(const ext_client::msvc9::wstring& message) -> void;
};
