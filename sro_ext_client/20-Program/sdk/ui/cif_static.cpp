#include "pch.hpp"
#include "sdk/ui/cif_static.hpp"

#include "sdk/ui/cif_decorated_static.hpp"
#include "sdk/process/cps_outer_interface.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstring>
#include <cwchar>

namespace {

  using ext_client::off::as_fn;
} // namespace

auto cif_static::set_visible(bool visible) -> int {
  return cgwnd::set_visible(this, visible);
}

auto cif_static::set_text(const wchar_t* text) -> char {
  const auto fn = as_fn<cif_static_fn::set_text>(0x00729570);
  return fn(this, text);
}

auto cif_static::set_text(const ext_client::msvc9::wstring& text) -> char {
  return set_text(text.data());
}

auto cif_static::set_text_color(std::uint32_t sro_color) -> int {
  using set_text_color_fn = int(__thiscall*)(cif_text_color_state * self, std::uint32_t color);
  const auto fn = as_fn<set_text_color_fn>(0x00B13630);
  return fn(&ext_client::off::field_at<cif_text_color_state>(this, 0x090), sro_color);
}

auto cif_static::get_text_mode() const -> int {
  return ext_client::off::field_at<int>(this, 0x374);
}

auto cif_static::set_text_mode(int mode) -> void {
  ext_client::off::field_at<int>(this, 0x374) = mode;
}

auto cif_static::text_extent_w() const -> int {
  const int width = static_cast<int>(ext_client::off::field_at<float>(this, 0x38C) - ext_client::off::field_at<float>(this, 0x384));
  return width > 0 ? width : 0;
}

auto cif_static::is_ellipsis_hover_enabled() const -> bool {
  return ext_client::off::field_at<int>(this, 0x374) == k_set_text_mode_ellipsis;
}

auto cif_static::show_full_text_no_hover() -> void {
  ext_client::off::field_at<int>(this, 0x374) = k_set_text_mode_plain;
  ext_client::off::field_at<int>(this, 0x378) = 0;

  wchar_t full_text[512]{};
  if (text(full_text, 512) && full_text[0] != L'\0') {
    set_text(full_text);
  }

  refresh_layout();

  int extent = text_extent_w();
  if (extent <= 0) {
    const std::size_t len = full_text[0] != L'\0' ? wcslen(full_text) : 0;
    const int glyph_w = ext_client::off::field_at<std::int16_t>(this, 0x0E4) > 0 ? ext_client::off::field_at<std::int16_t>(this, 0x0E4) : 7;
    extent = static_cast<int>(len) * glyph_w;
  }

  if (extent > 0) {
    const int padded = extent + 12;
    if (padded > ext_client::off::field_at<int>(this, 0x048)) {
      set_size(padded, ext_client::off::field_at<int>(this, 0x04C));
    }
    ext_client::off::field_at<int>(this, 0x37C) = ext_client::off::field_at<int>(this, 0x048);
  }

  refresh_layout();
}

auto cif_static::enable_ellipsis_hover(int clip_width) -> void {
  const int width = clip_width > 0 ? clip_width : k_default_ellipsis_clip_width;

  ext_client::off::field_at<int>(this, 0x378) = 0;
  set_size(width, ext_client::off::field_at<int>(this, 0x04C));
  ext_client::off::field_at<int>(this, 0x37C) = width;

  wchar_t full_text[512]{};
  if (!text(full_text, 512) || full_text[0] == L'\0') {
    ext_client::off::field_at<int>(this, 0x374) = k_set_text_mode_ellipsis;
    refresh_layout();
    return;
  }

  ext_client::off::field_at<int>(this, 0x374) = k_set_text_mode_ellipsis;
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
  const auto fn = as_fn<cif_static_fn::update_text_layout>(0x00729600);
  return fn(this);
}

auto cif_static::set_text_fmt(const wchar_t* fmt, int major, int minor) -> char {
  const auto fn = as_fn<cif_static_fn::set_text_fmt_int>(0x007294D0);
  return fn(this, fmt, major, minor);
}

auto cif_static::set_text_fmt(const wchar_t* fmt, double value) -> char {
  const auto fn = as_fn<cif_static_fn::set_text_fmt_double>(0x007294D0);
  return fn(this, fmt, value);
}

auto cif_static::text(wchar_t* dst, std::size_t dst_count) const -> bool {
  if (!dst || dst_count == 0) {
    return false;
  }
  const auto* text_obj = reinterpret_cast<const std::uint8_t*>(this) + 0x1A4;
  return ext_client::msvc9::wstring_ref::from(text_obj).copy_to(dst, dst_count);
}

auto cif_static::set_align_h(int align) -> void {
  ext_client::off::field_at<int>(this, 0x088) = align;
}

auto cif_static::create_outer_wnd(cps_outer_interface* parent, void* res_descriptor, const cgwnd_create_rect& rect,
                                  int create_mode, int user_flags) -> cif_static* {
  using create_outer_wnd_fn =
    cgwnd*(__cdecl*)(cgwnd * parent, void* res_descriptor, const cgwnd_create_rect* rect, int create_mode, int user_flags);
  const auto fn = as_fn<create_outer_wnd_fn>(0x00D62410);
  auto* created = fn(reinterpret_cast<cgwnd*>(parent), res_descriptor, &rect, create_mode, user_flags);
  return created ? reinterpret_cast<cif_static*>(created) : nullptr;
}

auto cif_static::version_label_res() -> void* {
  return reinterpret_cast<void*>(0x01179970);
}

auto cif_static::loading_banner_res() -> void* {
  return reinterpret_cast<void*>(0x01179A58);
}

auto cif_static::is_static(const cgwnd* wnd) -> bool {
  if (!wnd) {
    return false;
  }
  return ext_client::gfx_runtime::is_class_name_match(wnd, "CIFStatic") ||
         ext_client::gfx_runtime::is_class_name_match(wnd, "CIFDecoratedStatic");
}

auto cif_static::static_label(cgwnd* wnd) -> cif_static* {
  if (!wnd) {
    return nullptr;
  }

  if (ext_client::gfx_runtime::is_class_name_match(wnd, "CIFDecoratedStatic")) {
    return reinterpret_cast<cif_static*>(reinterpret_cast<std::uint8_t*>(wnd) + 0x084);
  }

  if (ext_client::gfx_runtime::is_class_name_match(wnd, "CIFStatic")) {
    return reinterpret_cast<cif_static*>(wnd);
  }

  return nullptr;
}

auto cif_static::read_text(const cgwnd* wnd, wchar_t* dst, std::size_t dst_count) -> bool {
  if (!wnd || !dst || dst_count == 0) {
    return false;
  }

  auto* label = static_label(const_cast<cgwnd*>(wnd));
  if (!label) {
    return false;
  }

  const auto* text_obj = reinterpret_cast<const std::uint8_t*>(label) + 0x1A4;
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

  if (ext_client::gfx_runtime::is_class_name_match(wnd, "CIFDecoratedStatic")) {
    if (read_string_field(0x414)) {
      return true;
    }
    return read_string_field(0x418);
  }

  if (!is_static(wnd)) {
    return false;
  }

  return read_string_field(0x168);
}

auto cif_static::set_texture_path(const char* path) -> bool {
  if (!path || path[0] == '\0') {
    return false;
  }

  auto* image_sub = get_textboard();
  if (!image_sub) {
    return false;
  }
  const auto* image_vftable = *reinterpret_cast<void***>(image_sub);
  if (!image_vftable) {
    return false;
  }

  using set_texture_fn = void(__thiscall*)(void*, ext_client::msvc9::string_pod, int, int);
  const auto set_tex_fn =
    reinterpret_cast<set_texture_fn>(image_vftable[13/*set_texture*/]);
  if (!set_tex_fn) {
    return false;
  }

  ext_client::msvc9::string s(path);
  ext_client::msvc9::string_pod pod{};
  std::memcpy(&pod, s.raw(), sizeof(pod));

  ext_client::msvc9::string empty_s;
  std::memcpy(s.raw(), empty_s.raw(), sizeof(pod));

  set_tex_fn(image_sub, pod, 0, 0);
  return true;
}
