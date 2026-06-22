#pragma once

#include "cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

// CIFEdit: single-line text input (login fields, chat boxes). Size 0xB4B0.
class cif_edit : public cif_static {
public:
  DECLARE_SDK_WND_HELPERS(cif_edit, "CIFEdit")

  auto set_text(const wchar_t* text) -> char;
  auto set_text(const ext_client::msvc9::wstring& text) -> char;
  auto text(wchar_t* dst, std::size_t dst_count) const -> bool;

private:
  union {
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cif_edit::size - sizeof(cif_static)], "pad_end");
  };
};

