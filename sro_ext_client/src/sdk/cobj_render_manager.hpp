#pragma once

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
  union {
    DEFINE_MEMBER_0(std::uint32_t m_field_04, "field_04");
    DEFINE_MEMBER_N(std::uint32_t m_field_08, 0x04);
    DEFINE_MEMBER_N(void** m_vec_a_begin, 0x34);
    DEFINE_MEMBER_N(void** m_vec_a_end, 0x38);
    DEFINE_MEMBER_N(void** m_vec_a_cap, 0x3C);
    DEFINE_MEMBER_N(void** m_vec_b_begin, 0x4C);
    DEFINE_MEMBER_N(void** m_vec_b_end, 0x50);
    DEFINE_MEMBER_N(void** m_vec_b_cap, 0x54);
    DEFINE_MEMBER_N(std::uint32_t m_field_5c, 0x58);
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cobj_render_manager::size - sizeof(void*)], "pad_end");
  };
};

