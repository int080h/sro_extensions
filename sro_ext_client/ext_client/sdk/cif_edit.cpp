#include "cif_edit.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto cif_edit::set_text(const wchar_t* text) -> char {
  using set_text_fn = char(__thiscall*)(cif_edit * self, const wchar_t* text);
  const auto fn = as_fn<set_text_fn>(ext_client::offsets::cif_edit::functions::set_text);
  return fn(this, text);
}

auto cif_edit::set_text(const ext_client::msvc9::wstring& text) -> char {
  return set_text(text.data());
}

auto cif_edit::text(wchar_t* dst, std::size_t dst_count) const -> bool {
  return cif_static::text(dst, dst_count);
}
