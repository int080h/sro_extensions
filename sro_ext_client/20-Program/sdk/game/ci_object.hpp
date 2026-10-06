#pragma once

#include "sdk/game/ci_entity.hpp"
#include "sdk/game/c_animation_callback.hpp"
#include "sdk/types/s_position.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CIObject — Renderable Game Object base class
// Extends CIEntity @ +0x00 and CAnimationCallback MI @ +0x84
// Manages world 3D position coordinates, bounding boxes, scaling, and alpha.
// ---------------------------------------------------------------------------
class ci_object : public ci_entity, public c_animation_callback {
public:
  // 1. Position & Coordinates
  auto get_coords() const -> s_position;
  auto set_coords(const s_position& val) -> void;

  // 2. Bounding Box Dimensions
  auto get_bound_min_x() const -> float;
  auto get_bound_min_y() const -> float;
  auto get_bound_min_z() const -> float;
  auto get_bound_max_x() const -> float;
  auto get_bound_max_y() const -> float;
  auto get_bound_max_z() const -> float;

  auto set_bound_min_x(float val) -> void;
  auto set_bound_min_y(float val) -> void;
  auto set_bound_min_z(float val) -> void;
  auto set_bound_max_x(float val) -> void;
  auto set_bound_max_y(float val) -> void;
  auto set_bound_max_z(float val) -> void;

  // 3. Scaling & Rendering
  auto get_scale_x() const -> float;
  auto get_scale_y() const -> float;
  auto get_scale_z() const -> float;
  auto get_rendering_mode() const -> std::uint8_t;
  auto get_alpha_fade() const -> float;

  auto set_scale_x(float val) -> void;
  auto set_scale_y(float val) -> void;
  auto set_scale_z(float val) -> void;
  auto set_rendering_mode(std::uint8_t val) -> void;
  auto set_alpha_fade(float val) -> void;
};
