#pragma once

#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

class cobj_render_manager {
public:
  static auto is_instance(const void* ptr) -> bool;

  auto vec_a_begin() const -> void* const* { return m_vec_a_begin; }
  auto vec_a_end() const -> void* const* { return m_vec_a_end; }
  auto vec_b_begin() const -> void* const* { return m_vec_b_begin; }
  auto vec_b_end() const -> void* const* { return m_vec_b_end; }

  void* vftable;
  std::uint32_t m_field_04;
  std::uint32_t m_field_08;
  PAD_TO(0x0C, ext_client::offsets::cobj_render_manager::fields::vec_a_begin);
  void** m_vec_a_begin;
  void** m_vec_a_end;
  void** m_vec_a_cap;
  PAD_TO(ext_client::offsets::cobj_render_manager::fields::vec_a_cap + sizeof(void*),
         ext_client::offsets::cobj_render_manager::fields::vec_b_begin);
  void** m_vec_b_begin;
  void** m_vec_b_end;
  void** m_vec_b_cap;
  std::uint32_t m_field_5c;
  PAD_TO(ext_client::offsets::cobj_render_manager::fields::field_5c + sizeof(std::uint32_t),
         ext_client::offsets::cobj_render_manager::size);
};

static_assert(sizeof(cobj_render_manager) == ext_client::offsets::cobj_render_manager::size, "cobj_render_manager size mismatch");
static_assert(offsetof(cobj_render_manager, m_vec_a_begin) == ext_client::offsets::cobj_render_manager::fields::vec_a_begin);
static_assert(offsetof(cobj_render_manager, m_vec_b_begin) == ext_client::offsets::cobj_render_manager::fields::vec_b_begin);
