#include "cgame_text.hpp"
#include "utils/offsets.hpp"

namespace {
  using get_game_text_fn = const void*(__thiscall*)(void* self, const wchar_t* key);
}

auto cgame_text::get() -> cgame_text* {
  return reinterpret_cast<cgame_text*>(ext_client::offsets::cgame_text::globals::address);
}

auto cgame_text::get_text(const wchar_t* key) -> ext_client::msvc9::wstring_ref {
  auto fn = reinterpret_cast<get_game_text_fn>(ext_client::offsets::cgame_text::functions::get_game_text);
  const void* wstr_obj = fn(this, key);
  return ext_client::msvc9::wstring_ref::from(wstr_obj);
}
