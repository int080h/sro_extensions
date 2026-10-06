#pragma once

#include "sdk/ui/alram_entry.hpp"

#include <cstddef>

// ---------------------------------------------------------------------------
// alram_data — Alarm data container holding array of alram_entry slots
// Accessed via CIFMainPopup::GetAlram() @ 0x00781E50
// ---------------------------------------------------------------------------
struct alram_data {
  // 1. Entry Accessors
  auto get_entry(std::size_t index) -> alram_entry*;
  auto get_entry(std::size_t index) const -> const alram_entry*;

  static auto get_entry_at_raw(alram_data* data, std::size_t index) -> alram_entry*;
  static auto get_entry_at_raw(const alram_data* data, std::size_t index) -> const alram_entry*;
};
