#include "pch.hpp"
#include "sdk/game/ctext_string_manager.hpp"

namespace {
  using get_game_text_fn = const void*(__thiscall*)(void* self, const wchar_t* key);
}

auto ctext_string_manager::get() -> ctext_string_manager* {
  return reinterpret_cast<ctext_string_manager*>(0x0117EDA8);
}

auto ctext_string_manager::get_text(const wchar_t* key) -> ext_client::msvc9::wstring_ref {
  if (!this || !key) {
    return {};
  }
  __try {
    auto fn = reinterpret_cast<get_game_text_fn>(0x009E4F00);
    const void* wstr_obj = fn(this, key);
    if (!wstr_obj) {
      return {};
    }
    return ext_client::msvc9::wstring_ref::from(wstr_obj);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return {};
  }
}
