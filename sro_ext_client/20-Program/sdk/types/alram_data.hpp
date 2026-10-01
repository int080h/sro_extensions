#pragma once

#include "sdk/types/alram_entry.hpp"
#include <cstddef>

struct alram_data {
  auto get_entry(std::size_t index) -> alram_entry*;
  auto get_entry(std::size_t index) const -> const alram_entry*;

  static auto get_entry_at_raw(alram_data* data, std::size_t index) -> alram_entry*;
  static auto get_entry_at_raw(const alram_data* data, std::size_t index) -> const alram_entry*;
};
