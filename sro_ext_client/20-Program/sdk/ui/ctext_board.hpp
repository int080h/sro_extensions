#pragma once

#include "sdk/ui/cg_font_texture.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CTextBoard — Secondary multiple-inheritance base @ +0x84 on CIFWnd
// Native VTable: 0x00FF47F4 | Class Size: 0x140
// Handles texture surfaces, font texture binding, and text rendering paths.
// ---------------------------------------------------------------------------
class ctext_board {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x00FF47F4;
  static constexpr std::size_t   k_class_size  = 0x0140;

  // 1. Texture Surface & Paths
  [[nodiscard]] auto get_texture() const -> void* {
    return ext_client::off::field_at<void*>(this, 0x0E0);
  }

  [[nodiscard]] auto get_texture_path() const -> const char* {
    return ext_client::msvc9::string_ref::from(reinterpret_cast<const std::uint8_t*>(this) + 0x0E4).data();
  }

  auto copy_texture_path(char* out, std::size_t max_len) const -> bool {
    return ext_client::msvc9::string_ref::from(reinterpret_cast<const std::uint8_t*>(this) + 0x0E4).copy_to(out, max_len);
  }
};
