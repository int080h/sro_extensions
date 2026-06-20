#include "sdk/sworld.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::field_at;
  using ext_client::offsets::global_at;

} // namespace

auto sworld::instance() -> sworld* {
  return &global_at<sworld>(ext_client::offsets::sworld::globals::address);
}

auto sworld::is_instance() -> bool {
  auto* world = instance();
  if (!world) {
    return false;
  }
  return world->vftable != nullptr;
}

auto sworld::cell_limit() const -> std::int32_t {
  return field_at<std::int32_t>(this, ext_client::offsets::sworld::fields::cell_limit);
}

auto sworld::sight_range() const -> float {
  return field_at<float>(this, ext_client::offsets::sworld::fields::sight_range);
}

auto sworld::option(unsigned int index) const -> std::int32_t {
  if (index >= ext_client::offsets::sworld::fields::options_count) {
    return 0;
  }
  return field_at<std::int32_t>(this, ext_client::offsets::sworld::fields::options + index * sizeof(std::int32_t));
}

auto sworld::set_cell_limit(std::int32_t limit) -> void {
  field_at<std::int32_t>(this, ext_client::offsets::sworld::fields::cell_limit) = limit;
}

auto sworld::set_sight_range(float range) -> void {
  field_at<float>(this, ext_client::offsets::sworld::fields::sight_range) = range;
}

auto sworld::set_option(unsigned int index, std::int32_t value) -> void {
  if (index >= ext_client::offsets::sworld::fields::options_count) {
    return;
  }
  field_at<std::int32_t>(this, ext_client::offsets::sworld::fields::options + index * sizeof(std::int32_t)) = value;
}

auto sworld::set_graphics_option(unsigned int setting_id, int value) -> int {
  auto* vft = world_vftable();
  if (!vft || !vft->set_graphics_option) {
    return 0;
  }
  return vft->set_graphics_option(this, setting_id, value);
}
