#pragma once

#include "utils/msvc9_stl.hpp"

// CSROTextData / CTextStringManager (global at 0x0117EDA8).
class ctext_string_manager {
public:
  // Resolves UIIT_STT_* game text keys. Returns wstring_ref view.
  auto get_text(const wchar_t* key) -> ext_client::msvc9::wstring_ref;

  static auto get() -> ctext_string_manager*;
};
