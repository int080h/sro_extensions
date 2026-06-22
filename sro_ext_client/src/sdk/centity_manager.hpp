#pragma once

#include "cobj.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

class ci_charactor;

using entity_map = std::n_map<int, ci_charactor*>;

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
  union {
    DEFINE_MEMBER_N(int m_field_0c, 0x08);
    DEFINE_MEMBER_N(std::n_list<void*> m_list, 0x10);
    DEFINE_MEMBER_N(std::uint32_t m_field_20, 0x1C);
    DEFINE_MEMBER_N(entity_map m_entities_a, 0x20);
    DEFINE_MEMBER_N(entity_map m_entities_b, 0x2C);
    DEFINE_MEMBER_N(std::uint8_t m_flag_a, 0x38);
    DEFINE_MEMBER_N(std::uint8_t m_flag_b, 0x39);
    DEFINE_MEMBER_N(std::uint32_t m_uid_capacity, 0x3C);
    DEFINE_MEMBER_N(void* m_uid_buffer, 0x40);
    DEFINE_MEMBER_N(std::n_vector<ci_charactor*> m_entities, 0x3A0);
  };
};

