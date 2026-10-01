#pragma once

#include <cstdint>

struct cd3d_enumeration {
  auto is_is_alpha() -> bool;

  auto get_adapter_list() -> void*;
  auto get_min_width() -> std::uint32_t;
  auto get_min_height() -> std::uint32_t;
  auto get_min_backbuffer_format() -> std::uint32_t;
  auto get_min_refresh() -> std::uint32_t;
  auto get_depth_bits() -> std::uint32_t;
  auto get_multisample_type() -> std::uint32_t;
  auto get_can_do_windowed() -> std::uint8_t;
  auto get_is_stereo() -> std::uint8_t;
  auto get_device_combo() -> void*;

  auto set_is_alpha(bool val) -> void;
  auto set_adapter_list(void* val) -> void;
  auto set_min_width(std::uint32_t val) -> void;
  auto set_min_height(std::uint32_t val) -> void;
  auto set_min_backbuffer_format(std::uint32_t val) -> void;
  auto set_min_refresh(std::uint32_t val) -> void;
  auto set_depth_bits(std::uint32_t val) -> void;
  auto set_multisample_type(std::uint32_t val) -> void;
  auto set_can_do_windowed(std::uint8_t val) -> void;
  auto set_is_stereo(std::uint8_t val) -> void;
  auto set_device_combo(void* val) -> void;
};
