#pragma once

#include "sdk/ui/cif_static.hpp"
#include "utils/msvc9_stl.hpp"

// ---------------------------------------------------------------------------
// CIFEdit — Single-line text input widget (extends CIFStatic)
// Class Size: 0xB4B0
// ---------------------------------------------------------------------------
class cif_edit : public cif_static {
public:
  static constexpr std::size_t k_class_size = 0xB4B0;

  // 1. Text Content & Input
  auto set_text(const wchar_t* text) -> char;
  auto set_text(const ext_client::msvc9::wstring& text) -> char;
  auto text(wchar_t* dst, std::size_t dst_count) const -> bool;
};
