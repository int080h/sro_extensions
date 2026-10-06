#pragma once

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// SWorld — In-game world / terrain / graphics state (singleton @ 0x013AB480)
// Native VTable: 0x01044524 (??_7SWorld@@6B@)
// Per-field offsets: ext_client::off::sworld in offsets/sworld.hpp
// ---------------------------------------------------------------------------
class sworld {
public:
  static constexpr std::uint32_t k_vtable_addr    = 0x01044524;
  static constexpr std::uint32_t k_singleton_addr = 0x013AB480;

  // 1. Singleton & Type Inspection
  static auto instance() -> sworld*;
  static auto is_instance() -> bool;
  static auto cast(void* ptr) -> sworld*;
  static auto cast(const void* ptr) -> const sworld*;

  // 2. World & Graphic Settings (Getters)
  auto cell_limit() const -> std::int32_t;
  auto sight_range() const -> float;
  auto option(unsigned int index) const -> std::int32_t;

  // 3. World & Graphic Settings (Setters)
  auto set_cell_limit(std::int32_t limit) -> void;
  auto set_sight_range(float range) -> void;
  auto set_option(unsigned int index, std::int32_t value) -> void;
  auto set_graphics_option(unsigned int setting_id, int value) -> int;

  // 4. Render Pipeline Callbacks
  auto set_render_callback(int(__cdecl* callback)(int)) -> void;
  static auto intro_render_stage_callback() -> int(__cdecl*)(int);
};
