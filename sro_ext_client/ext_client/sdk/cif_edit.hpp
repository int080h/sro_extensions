#pragma once

#include "cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

// CIFEdit: single-line text input (login fields, chat boxes). Size 0xB4B0.
class cif_edit : public cif_static {
public:
  PAD_TO(ext_client::offsets::cif_static::size, ext_client::offsets::cif_edit::size);

  DECLARE_SDK_WND_HELPERS(cif_edit, ext_client::offsets::cif_edit::vtable::address)

  auto set_text(const wchar_t* text) -> char;
  auto set_text(const ext_client::msvc9::wstring& text) -> char;
  auto text(wchar_t* dst, std::size_t dst_count) const -> bool;
};

static_assert(sizeof(cif_edit) == ext_client::offsets::cif_edit::size, "cif_edit size mismatch");
