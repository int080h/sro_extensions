#pragma once

#include "sdk/game/cobj_child.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

class ci_charactor;

class ci_entity;

using entity_map = std::n_map<int, ci_charactor*>;

// CEntityManager — CObjChild @ +0x00, manager fields from +0x20.
class centity_manager : public cobj_child {
public:
  auto is_flag_a() -> bool;
  auto is_flag_b() -> bool;

  auto get_field_20() -> int;
  auto get_uid_capacity() -> std::uint32_t;
  auto get_uid_buffer() -> void*;
  auto get_entity_vec_begin() -> ci_charactor**;
  auto get_entity_begin() const -> ci_charactor* const*;
  auto get_entity_end() const -> ci_charactor* const*;
  auto get_entity_count() const -> std::size_t;

  auto set_flag_a(bool val) -> void;
  auto set_flag_b(bool val) -> void;
  auto set_field_20(int val) -> void;
  auto set_uid_capacity(std::uint32_t val) -> void;
  auto set_uid_buffer(void* val) -> void;
  auto set_entity_vec_begin(ci_charactor** val) -> void;

  // Game lookup returns character pointers; use as_entity() for ci_entity* view.
  auto lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor*;
  auto lookup_entity(std::uint32_t slot_id) const -> ci_entity*;

  static auto is_instance(const void* ptr) -> bool;

  static auto get_singleton() -> centity_manager*;
};
