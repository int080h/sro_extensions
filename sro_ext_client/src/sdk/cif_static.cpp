#include "cif_static.hpp"

#include "cif_decorated_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/rtti.hpp"

#include <cstring>
#include <cwchar>

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto cif_static::set_visible(bool visible) -> int {
  return cgwnd::set_visible(this, visible);
}

auto cif_static::set_text(const wchar_t* text) -> char {
  return static_vftable()->set_text(this, text);
}

auto cif_static::set_text(const ext_client::msvc9::wstring& text) -> char {
  return set_text(text.data());
}

auto cif_static::set_text_color(std::uint32_t sro_color) -> int {
  using set_text_color_fn = int(__thiscall*)(cif_text_color_state * self, std::uint32_t color);
  const auto fn = as_fn<set_text_color_fn>(ext_client::offsets::cgwnd::functions::set_text_color);
  return fn(&m_text_color_state, sro_color);
}

auto cif_static::text_mode() const -> int {
  return m_set_text_mode;
}

auto cif_static::set_text_mode(int mode) -> void {
  m_set_text_mode = mode;
}

auto cif_static::text_extent_w() const -> int {
  const int width = static_cast<int>(m_text_bounds_w - m_text_bounds_x);
  return width > 0 ? width : 0;
}

auto cif_static::ellipsis_hover_enabled() const -> bool {
  return m_set_text_mode == k_set_text_mode_ellipsis;
}

auto cif_static::show_full_text_no_hover() -> void {
  m_set_text_mode = k_set_text_mode_plain;
  m_text_flags = 0;

  wchar_t full_text[512]{};
  if (text(full_text, 512) && full_text[0] != L'\0') {
    set_text(full_text);
  }

  refresh_layout();

  int extent = text_extent_w();
  if (extent <= 0) {
    const std::size_t len = full_text[0] != L'\0' ? wcslen(full_text) : 0;
    const int glyph_w = m_font_width > 0 ? m_font_width : 7;
    extent = static_cast<int>(len) * glyph_w;
  }

  if (extent > 0) {
    const int padded = extent + 12;
    if (padded > m_rect_w) {
      set_size(padded, m_rect_h);
    }
    m_saved_rect_w = m_rect_w;
  }

  refresh_layout();
}

auto cif_static::enable_ellipsis_hover(int clip_width) -> void {
  const int width = clip_width > 0 ? clip_width : k_default_ellipsis_clip_width;

  m_text_flags = 0;
  set_size(width, m_rect_h);
  m_saved_rect_w = width;

  wchar_t full_text[512]{};
  if (!text(full_text, 512) || full_text[0] == L'\0') {
    m_set_text_mode = k_set_text_mode_ellipsis;
    refresh_layout();
    return;
  }

  m_set_text_mode = k_set_text_mode_ellipsis;
  set_text(full_text);
  refresh_layout();
}

auto cif_static::set_text_clip_mode(cif_text_clip_mode mode, int ellipsis_width) -> void {
  switch (mode) {
    case cif_text_clip_mode::full:
      show_full_text_no_hover();
      break;
    case cif_text_clip_mode::ellipsis_hover:
      enable_ellipsis_hover(ellipsis_width);
      break;
  }
}

auto cif_static::refresh_layout() -> int {
  return static_vftable()->update_text_layout(this);
}

auto cif_static::set_text_fmt(const wchar_t* fmt, int major, int minor) -> char {
  return static_vftable()->set_text_fmt(this, fmt, major, minor);
}

auto cif_static::set_text_fmt(const wchar_t* fmt, double value) -> char {
  return static_vftable()->set_text_fmt(this, fmt, value);
}

auto cif_static::text(wchar_t* dst, std::size_t dst_count) const -> bool {
  if (!dst || dst_count == 0) {
    return false;
  }
  const auto* text_obj = reinterpret_cast<const std::uint8_t*>(this) + ext_client::offsets::cif_static::fields::text_wstring;
  return ext_client::msvc9::wstring_ref::from(text_obj).copy_to(dst, dst_count);
}

auto cif_static::create_outer_wnd(cgwnd* parent, void* res_descriptor, const cgwnd_create_rect& rect, int create_mode, int user_flags)
  -> cif_static* {
  using create_outer_wnd_fn =
    cgwnd*(__cdecl*)(cgwnd * parent, void* res_descriptor, const cgwnd_create_rect* rect, int create_mode, int user_flags);
  const auto fn = as_fn<create_outer_wnd_fn>(ext_client::offsets::cgwnd::functions::create_outer_wnd);
  return cif_static::from(fn(parent, res_descriptor, &rect, create_mode, user_flags));
}

auto cif_static::version_label_res() -> void* {
  return reinterpret_cast<void*>(ext_client::offsets::cgwnd::globals::version_label_res);
}

auto cif_static::is_static(const cgwnd* wnd) -> bool {
  if (!wnd) {
    return false;
  }
  const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
  if (vftable == ext_client::offsets::cif_static::vtable::address ||
      vftable == ext_client::offsets::cif_static::vtable::secondary) {
    return true;
  }
  if (vftable == ext_client::offsets::cif_decorated_static::vtable::address ||
      vftable == ext_client::offsets::cif_decorated_static::vtable::secondary) {
    return true;
  }
  // RTTI fallback for unknown vtables
  char name[64]{};
  if (ext_client::rtti::class_name(vftable, name, sizeof(name))) {
    return std::strstr(name, "IFStatic") != nullptr;
  }
  return false;
}

auto cif_static::as_if_static(cgwnd* wnd) -> cif_static* {
  if (!wnd) {
    return nullptr;
  }

  const auto vftable = reinterpret_cast<std::uint32_t>(wnd->vftable);
  if (vftable == ext_client::offsets::cif_decorated_static::vtable::address ||
      vftable == ext_client::offsets::cif_decorated_static::vtable::secondary) {
    return reinterpret_cast<cif_static*>(reinterpret_cast<std::uint8_t*>(wnd) + ext_client::offsets::cif_static::subobject_offset);
  }

  if (vftable == ext_client::offsets::cif_static::vtable::address ||
      vftable == ext_client::offsets::cif_static::vtable::secondary) {
    return cif_static::from(wnd);
  }

  if (!wnd->is_live()) {
    return nullptr;
  }

  char name[64]{};
  if (!ext_client::rtti::class_name(vftable, name, sizeof(name))) {
    return nullptr;
  }
  if (std::strstr(name, "IFStatic") == nullptr) {
    return nullptr;
  }
  // Decorated static subobject or plain static
  if (std::strstr(name, "Decorated") != nullptr) {
    return reinterpret_cast<cif_static*>(reinterpret_cast<std::uint8_t*>(wnd) + ext_client::offsets::cif_static::subobject_offset);
  }
  return cif_static::from(wnd);
}

auto cif_static::read_text(const cgwnd* wnd, wchar_t* dst, std::size_t dst_count) -> bool {
  if (!wnd || !dst || dst_count == 0) {
    return false;
  }

  auto* label = as_if_static(const_cast<cgwnd*>(wnd));
  if (!label) {
    return false;
  }

  const auto* text_obj = reinterpret_cast<const std::uint8_t*>(label) + ext_client::offsets::cif_static::fields::text_wstring;
  return ext_client::msvc9::wstring_ref::from(text_obj).copy_to(dst, dst_count);
}

auto cif_static::read_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool {
  if (!wnd || !dst || dst_count == 0) {
    return false;
  }
  dst[0] = '\0';

  auto read_string_field = [&](std::size_t byte_offset) -> bool {
    const auto* field = reinterpret_cast<const std::uint8_t*>(wnd) + byte_offset;
    return ext_client::msvc9::string_ref::from(field).copy_to(dst, dst_count) && dst[0] != '\0';
  };

  if (cif_decorated_static::is_instance(wnd)) {
    if (read_string_field(ext_client::offsets::cif_decorated_static::fields::texture_path_alt)) {
      return true;
    }
    return read_string_field(ext_client::offsets::cif_decorated_static::fields::texture_path);
  }

  if (!is_static(wnd)) {
    return false;
  }

  return read_string_field(ext_client::offsets::cif_static::fields::image_texture_path);
}
