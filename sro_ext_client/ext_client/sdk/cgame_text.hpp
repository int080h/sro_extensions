#pragma once

#include "utils/msvc9_stl.hpp"

// CSROTextData / CGameTextManager (global at ext_client::offsets::cgame_text::globals::address).
class cgame_text {
public:
  static auto get() -> cgame_text*;

  // Resolves UIIT_STT_* game text keys. Returns wstring_ref view.
  auto get_text(const wchar_t* key) -> ext_client::msvc9::wstring_ref;
};
