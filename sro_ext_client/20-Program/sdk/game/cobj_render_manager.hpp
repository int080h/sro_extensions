#pragma once


#include <cstdint>

class cobj_render_manager {
public:
  auto get_field_04() -> std::uint32_t;
  auto get_field_08() -> std::uint32_t;
  auto get_vec_a_begin() -> void**;
  auto get_vec_a_end() -> void**;
  auto get_vec_a_cap() -> void**;
  auto get_vec_b_begin() -> void**;
  auto get_vec_b_end() -> void**;
  auto get_vec_b_cap() -> void**;
  auto get_field_5c() -> std::uint32_t;

  auto set_field_04(std::uint32_t val) -> void;
  auto set_field_08(std::uint32_t val) -> void;
  auto set_vec_a_begin(void** val) -> void;
  auto set_vec_a_end(void** val) -> void;
  auto set_vec_a_cap(void** val) -> void;
  auto set_vec_b_begin(void** val) -> void;
  auto set_vec_b_end(void** val) -> void;
  auto set_vec_b_cap(void** val) -> void;
  auto set_field_5c(std::uint32_t val) -> void;

  static auto is_instance(const void* ptr) -> bool;
};

