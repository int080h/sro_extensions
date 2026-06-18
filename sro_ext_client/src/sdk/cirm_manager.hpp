#pragma once

#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

class cirm_manager {
public:
  static auto get() -> cirm_manager*;
  static auto is_instance(const void* ptr) -> bool;

  auto raw() const -> const void* { return this; }

  void* vftable;
  std::uint8_t m_irm_subobject[0x04];
  void* m_map_head;
  PAD_TO(ext_client::offsets::cirm_manager::fields::map_head + sizeof(void*),
         ext_client::offsets::cirm_manager::fields::vec_begin);
  void** m_vec_begin;
  void** m_vec_end;
  void** m_vec_cap;
};

static_assert(offsetof(cirm_manager, m_irm_subobject) == ext_client::offsets::cirm_manager::fields::irm_subobject);
static_assert(offsetof(cirm_manager, m_vec_begin) == ext_client::offsets::cirm_manager::fields::vec_begin);
