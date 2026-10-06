#include "pch.hpp"
#include "sdk/game/sworld.hpp"

#include "utils/offsets.hpp"

namespace {
  using ext_client::off::as_fn;
  using ext_client::off::field_at;
  using ext_client::off::global_at;

  using set_graphics_option_fn = int(__thiscall*)(sworld* self, unsigned int setting_id, int value);
  using set_render_cb_fn = void(__stdcall*)(int(__cdecl*)(int));
} // namespace

// ===========================================================================
// 1. Singleton & Type Inspection
// ===========================================================================
auto sworld::instance() -> sworld* {
  return &global_at<sworld>(k_singleton_addr);
}

auto sworld::is_instance() -> bool {
  auto* world = instance();
  if (!world) {
    return false;
  }
  return field_at<std::uint32_t>(world, 0) == k_vtable_addr;
}

auto sworld::cast(void* ptr) -> sworld* {
  if (!ptr || field_at<std::uint32_t>(ptr, 0) != k_vtable_addr) {
    return nullptr;
  }
  return static_cast<sworld*>(ptr);
}

auto sworld::cast(const void* ptr) -> const sworld* {
  if (!ptr || field_at<std::uint32_t>(ptr, 0) != k_vtable_addr) {
    return nullptr;
  }
  return static_cast<const sworld*>(ptr);
}

// ===========================================================================
// 2. World & Graphic Settings (Getters)
// ===========================================================================
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

// ===========================================================================
// 3. World & Graphic Settings (Setters)
// ===========================================================================
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

// ===========================================================================
// 4. Render Pipeline Callbacks
// ===========================================================================
auto sworld::set_render_callback(int(__cdecl* callback)(int)) -> void {
  const auto fn = as_fn<set_render_cb_fn>(0x00B5CA50);
  if (fn) {
    fn(callback);
  }
}

auto sworld::intro_render_stage_callback() -> int(__cdecl*)(int) {
  return as_fn<int(__cdecl*)(int)>(0x0094D050);
}

