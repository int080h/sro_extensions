#pragma once

#include "cobj.hpp"
#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

class ci_charactor;

// CEntityManager — CObjChild prefix (0x20) + manager fields. Flat layout avoids duplicate
// cobj_child padding from MI header redeclaration.
class centity_manager {
public:
  static auto get_singleton() -> centity_manager*;
  static auto is_instance(const void* ptr) -> bool;

  auto as_cobj_child() -> cobj_child* { return reinterpret_cast<cobj_child*>(this); }
  auto as_cobj_child() const -> const cobj_child* { return reinterpret_cast<const cobj_child*>(this); }

  auto lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor*;
  auto entity_begin() const -> ci_charactor* const*;
  auto entity_end() const -> ci_charactor* const*;
  auto entity_count() const -> std::size_t;

  cobj_child_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj_child::fields::field_0c);
  int m_field_0c;
  PAD_TO(ext_client::offsets::cobj_child::fields::field_0c + sizeof(int),
         ext_client::offsets::cobj_child::fields::list_node);
  void* m_list_node[3];
  std::uint32_t m_field_20;
  PAD_TO(ext_client::offsets::centity_manager::fields::field_20 + sizeof(std::uint32_t),
         ext_client::offsets::centity_manager::fields::hash_map_a_head);
  void* m_hash_map_a_head;
  std::uint32_t m_hash_map_a_count;
  PAD_TO(ext_client::offsets::centity_manager::fields::hash_map_a_count + sizeof(std::uint32_t),
         ext_client::offsets::centity_manager::fields::hash_map_b_head);
  void* m_hash_map_b_head;
  std::uint32_t m_hash_map_b_count;
  std::uint8_t m_flag_a;
  std::uint8_t m_flag_b;
  PAD_TO(ext_client::offsets::centity_manager::fields::flag_b + sizeof(std::uint8_t),
         ext_client::offsets::centity_manager::fields::uid_capacity);
  std::uint32_t m_uid_capacity;
  void* m_uid_buffer;
  PAD_TO(ext_client::offsets::centity_manager::fields::uid_buffer + sizeof(void*),
         ext_client::offsets::centity_manager::fields::entity_vec_begin);
  ci_charactor** m_entity_begin;
  ci_charactor** m_entity_end;
};

static_assert(offsetof(centity_manager, m_field_0c) == ext_client::offsets::cobj_child::fields::field_0c);
static_assert(offsetof(centity_manager, m_field_20) == ext_client::offsets::centity_manager::fields::field_20);
static_assert(offsetof(centity_manager, m_entity_begin) == ext_client::offsets::centity_manager::fields::entity_vec_begin);
static_assert(offsetof(centity_manager, m_entity_end) == ext_client::offsets::centity_manager::fields::entity_vec_end);
