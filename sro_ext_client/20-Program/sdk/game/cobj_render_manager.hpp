#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// CObjRenderManager — Object Render Pipeline Manager
// Native VTable: 0x01077254
// ---------------------------------------------------------------------------
class cobj_render_manager {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01077254;

  // 1. Type Inspection
  static auto is_instance(const void* ptr) -> bool;
  static auto cast(void* ptr) -> cobj_render_manager*;
  static auto cast(const void* ptr) -> const cobj_render_manager*;

  // 2. Render Queue Vectors
  auto get_vec_a_begin() -> void**;
  auto get_vec_a_end() -> void**;
  auto get_vec_b_begin() -> void**;
  auto get_vec_b_end() -> void**;
};
