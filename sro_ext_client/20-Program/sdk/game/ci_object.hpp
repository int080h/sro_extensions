#pragma once

#include "sdk/game/ci_entity.hpp"
#include "sdk/game/c_animation_callback.hpp"
#include "sdk/types/s_position.hpp"

#include <cstdint>

// ci_object inherits from ci_entity and c_animation_callback at offset 84.
class ci_object : public ci_entity, public c_animation_callback {
public:
  auto get_alpha_fade() -> float;
  auto get_coords() -> s_position;
  auto get_bound_min_x() -> float;
  auto get_bound_min_y() -> float;
  auto get_bound_min_z() -> float;
  auto get_bound_max_x() -> float;
  auto get_bound_max_y() -> float;
  auto get_bound_max_z() -> float;
  auto get_rendering_mode() -> std::uint8_t;
  auto get_scale_x() -> float;
  auto get_scale_y() -> float;
  auto get_scale_z() -> float;
  auto set_alpha_fade(float val) -> void;
  auto set_coords(s_position val) -> void;
  auto set_bound_min_x(float val) -> void;
  auto set_bound_min_y(float val) -> void;
  auto set_bound_min_z(float val) -> void;
  auto set_bound_max_x(float val) -> void;
  auto set_bound_max_y(float val) -> void;
  auto set_bound_max_z(float val) -> void;
  auto set_rendering_mode(std::uint8_t val) -> void;
  auto set_scale_x(float val) -> void;
  auto set_scale_y(float val) -> void;
  auto set_scale_z(float val) -> void;
};
