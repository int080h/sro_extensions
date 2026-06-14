#pragma once

#include "calarm_entry.hpp"
#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstddef>

inline constexpr std::size_t calarm_entry_count = 13;

class calarm_data {
public:
  auto entry(std::size_t index) -> calarm_entry*;
  auto entry(std::size_t index) const -> const calarm_entry*;
  static auto entry_at_raw(calarm_data* data, std::size_t index) -> calarm_entry*;
  static auto entry_at_raw(const calarm_data* data, std::size_t index) -> const calarm_entry*;

private:
  PAD(ext_client::offsets::calarm_data::entries_begin);
  calarm_entry m_entries[1];
};
