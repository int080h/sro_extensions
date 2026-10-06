#pragma once

#include "utils/msvc9_stl.hpp"

#include <cstdint>

// ---------------------------------------------------------------------------
// CTextStringManager (CSROTextData) — Client localization string table manager
// Native Global: 0x0117EDA8 | String lookup @ 0x009E4F00
// Resolves UIIT_*, UIOT_* game text keys to localized wide strings.
// ---------------------------------------------------------------------------
class ctext_string_manager {
public:
  static constexpr std::uint32_t k_singleton_addr = 0x0117EDA8;
  static constexpr std::uint32_t k_vftable_addr   = 0x0103D478;

  // 1. Singleton Access
  static auto get() -> ctext_string_manager*;

  // 2. Localization Key Lookups
  auto get_text(const wchar_t* key) -> ext_client::msvc9::wstring_ref;
};
