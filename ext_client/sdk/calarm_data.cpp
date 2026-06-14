#include "calarm_data.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto calarm_data::entry(std::size_t index) -> calarm_entry* {
  using entry_at_fn = calarm_entry*(__thiscall*)(calarm_data * self, int index);
  const auto fn = as_fn<entry_at_fn>(ext_client::offsets::calarm_data::functions::entry_at);
  return fn(this, static_cast<int>(index));
}

auto calarm_data::entry(std::size_t index) const -> const calarm_entry* {
  return const_cast<calarm_data*>(this)->entry(index);
}

auto calarm_data::entry_at_raw(calarm_data* data, std::size_t index) -> calarm_entry* {
  if (!data || index >= calarm_entry_count) {
    return nullptr;
  }
  auto* base = reinterpret_cast<std::uint8_t*>(data) + ext_client::offsets::calarm_data::entries_begin;
  return reinterpret_cast<calarm_entry*>(base + index * ext_client::offsets::calarm_entry::stride);
}

auto calarm_data::entry_at_raw(const calarm_data* data, std::size_t index) -> const calarm_entry* {
  return entry_at_raw(const_cast<calarm_data*>(data), index);
}
