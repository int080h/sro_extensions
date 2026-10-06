#include "pch.hpp"
#include "sdk/game/ctext_string_manager.hpp"

#include "utils/offsets.hpp"

namespace {
  using get_game_text_fn = const void*(__thiscall*)(void* self, const wchar_t* key);
} // namespace

// ===========================================================================
// 1. Singleton Access
// ===========================================================================
auto ctext_string_manager::get() -> ctext_string_manager* {
  auto* mgr = reinterpret_cast<ctext_string_manager*>(k_singleton_addr);
  if (!ext_client::utils::memory::is_game_ptr(mgr)) {
    return nullptr;
  }
  // 0x0117EDA8 is the static global CTextStringManager object in .bss.
  // Verify that its vtable pointer is initialized and committed in engine memory.
  const auto vtable = *reinterpret_cast<const std::uintptr_t*>(mgr);
  if (!ext_client::utils::memory::is_game_ptr(reinterpret_cast<const void*>(vtable))) {
    return nullptr;
  }
  return mgr;
}

// ===========================================================================
// 2. Localization Key Lookups
// ===========================================================================
auto ctext_string_manager::get_text(const wchar_t* key) -> ext_client::msvc9::wstring_ref {
  if (!this || !key || !ext_client::utils::memory::is_game_ptr(this)) {
    return {};
  }
  const auto fn = reinterpret_cast<get_game_text_fn>(0x009E4F00);
  const void* wstr_obj = fn(this, key);
  if (!wstr_obj || !ext_client::utils::memory::is_game_ptr(wstr_obj)) {
    return {};
  }
  return ext_client::msvc9::wstring_ref::from(wstr_obj);
}
