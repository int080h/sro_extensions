#pragma once

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// alram_entry — Alarm slot descriptor entry in CIFMainPopup / CAlramGuideMgrWnd
// Native Size: 0x1F8 (504 bytes)
// ---------------------------------------------------------------------------
struct alram_entry {
  static constexpr std::size_t k_class_size = 0x1F8;

  // 1. Entry Inspection & Type
  auto is_active() const -> bool;
  auto is_facebook() const -> bool;
  auto is_magic_lamp() const -> bool;
  auto is_daily_login() const -> bool;
  auto is_hidden() const -> bool;

  auto get_type() const -> int;
  auto get_ref_data_ptr() -> void*;

  // 2. Modifiers
  auto set_active(bool active) -> void;
  auto set_type(int type) -> void;
};
