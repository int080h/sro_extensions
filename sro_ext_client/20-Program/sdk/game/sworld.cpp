#include "pch.hpp"
#include "sdk/game/sworld.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::field_at;
  using ext_client::off::global_at;

  using set_graphics_option_fn = int(__thiscall*)(sworld* self, unsigned int setting_id, int value);

} // namespace

auto sworld::instance() -> sworld* {
  return &global_at<sworld>(0x013AB480);
}

auto sworld::is_instance() -> bool {
  auto* world = instance();
  if (!world) {
    return false;
  }
  return field_at<void*>(world, 0) != nullptr;
}

auto sworld::cell_limit() const -> std::int32_t {
  return field_at<std::int32_t>(this, 0x23DB);
}

auto sworld::sight_range() const -> float {
  return field_at<float>(this, 0x2782);
}

auto sworld::option(unsigned int index) const -> std::int32_t {
  if (index >= 51/*options_count*/) {
    return 0;
  }
  return field_at<std::int32_t>(this, 0x2F20 + index * sizeof(std::int32_t));
}

auto sworld::set_cell_limit(std::int32_t limit) -> void {
  field_at<std::int32_t>(this, 0x23DB) = limit;
}

auto sworld::set_sight_range(float range) -> void {
  field_at<float>(this, 0x2782) = range;
}

auto sworld::set_option(unsigned int index, std::int32_t value) -> void {
  if (index >= 51/*options_count*/) {
    return;
  }
  field_at<std::int32_t>(this, 0x2F20 + index * sizeof(std::int32_t)) = value;
}

auto sworld::set_graphics_option(unsigned int setting_id, int value) -> int {
  const auto fn = as_fn<set_graphics_option_fn>(0x00B841E0);
  if (!fn) {
    return 0;
  }
  return fn(this, setting_id, value);
}
