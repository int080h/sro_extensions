#pragma once

#include "sdk/ui/cif_wnd.hpp"
#include "sdk/types/cif_text_clip_mode.hpp"
#include "sdk/types/cif_text_color_state.hpp"

#include <cstddef>
#include <cstdint>

namespace ext_client::msvc9 {
  class wstring;
}

class cps_outer_interface;

class cif_static;

inline constexpr int k_default_ellipsis_clip_width = 80;

inline constexpr int k_set_text_mode_ellipsis = 1;
inline constexpr int k_set_text_mode_plain = 0;

namespace cif_static_fn {

  using set_text = char(__thiscall*)(cif_static*, const wchar_t*);
  using set_text_fmt_int = char(__cdecl*)(cif_static*, const wchar_t*, int, int);
  using set_text_fmt_double = char(__cdecl*)(cif_static*, const wchar_t*, double);
  using update_text_layout = int(__thiscall*)(cif_static*);

} // namespace cif_static_fn

// CIFStatic: label / static text control (inherits CIFWnd, which inherits CGWnd + CTextBoard).
class cif_static : public cif_wnd {
public:
  auto is_ellipsis_hover_enabled() const -> bool;

  auto get_align_h() -> int;
  auto get_align_v() -> int;
  auto get_text_color_state() -> cif_text_color_state;
  auto get_line_buffer_begin() -> void*;
  auto get_line_buffer_end() -> void*;
  auto get_font_width() -> std::int16_t;
  auto get_font_height() -> std::int16_t;
  auto get_set_text_mode() -> int;
  auto get_text_flags() -> int;
  auto get_saved_rect_w() -> int;
  auto get_text_bounds_x() -> float;
  auto get_text_bounds_y() -> float;
  auto get_text_bounds_w() -> float;
  auto get_text_bounds_h() -> float;
  auto get_text_mode() const -> int;

  auto set_align_v(int val) -> void;
  auto set_text_color_state(cif_text_color_state val) -> void;
  auto set_line_buffer_begin(void* val) -> void;
  auto set_line_buffer_end(void* val) -> void;
  auto set_font_width(std::int16_t val) -> void;
  auto set_font_height(std::int16_t val) -> void;
  auto set_set_text_mode(int val) -> void;
  auto set_text_flags(int val) -> void;
  auto set_saved_rect_w(int val) -> void;
  auto set_text_bounds_x(float val) -> void;
  auto set_text_bounds_y(float val) -> void;
  auto set_text_bounds_w(float val) -> void;
  auto set_text_bounds_h(float val) -> void;
  auto set_visible(bool visible) -> int;
  auto set_text(const wchar_t* text) -> char;
  auto set_text(const ext_client::msvc9::wstring& text) -> char;
  // SRO color (0xAABBGGRR) via CIFTextColor_SetAll (+0x90). Does not touch set_text_mode (+0x374).
  auto set_text_color(std::uint32_t sro_color) -> int;
  auto set_align_h(int align) -> void;
  auto set_text_mode(int mode) -> void;
  auto set_text_clip_mode(cif_text_clip_mode mode, int ellipsis_width = k_default_ellipsis_clip_width) -> void;
  auto set_text_fmt(const wchar_t* fmt, int major, int minor) -> char;
  auto set_text_fmt(const wchar_t* fmt, double value) -> char;
  auto set_texture_path(const char* path) -> bool;

  auto refresh_layout() -> int;
  auto text_extent_w() const -> int;
  auto show_full_text_no_hover() -> void;
  auto enable_ellipsis_hover(int clip_width = k_default_ellipsis_clip_width) -> void;
  auto text(wchar_t* dst, std::size_t dst_count) const -> bool;

  static auto is_static(const cgwnd* wnd) -> bool;

  static auto static_label(cgwnd* wnd) -> cif_static*;
  static auto read_text(const cgwnd* wnd, wchar_t* dst, std::size_t dst_count) -> bool;
  static auto read_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool;
  static auto create_outer_wnd(cps_outer_interface* parent, void* res_descriptor, const cgwnd_create_rect& rect, int create_mode = 0, int user_flags = 0) -> cif_static*;
  static auto version_label_res() -> void*;
};

