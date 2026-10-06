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

// ---------------------------------------------------------------------------
// CIFStatic — Label / static text control (extends CIFWnd)
// Class Size: 0x394
// ---------------------------------------------------------------------------
class cif_static : public cif_wnd {
public:
  static constexpr std::size_t k_class_size = 0x0394;

  // 1. Text Content & Formatting
  auto set_text(const wchar_t* text) -> char;
  auto set_text(const ext_client::msvc9::wstring& text) -> char;
  auto set_text_fmt(const wchar_t* fmt, int major, int minor) -> char;
  auto set_text_fmt(const wchar_t* fmt, double value) -> char;
  auto text(wchar_t* dst, std::size_t dst_count) const -> bool;

  // 2. Ellipsis & Clipping Control
  auto get_text_mode() const -> int;
  auto set_text_mode(int mode) -> void;
  auto is_ellipsis_hover_enabled() const -> bool;
  auto show_full_text_no_hover() -> void;
  auto enable_ellipsis_hover(int clip_width = k_default_ellipsis_clip_width) -> void;
  auto set_text_clip_mode(cif_text_clip_mode mode, int ellipsis_width = k_default_ellipsis_clip_width) -> void;

  // 3. Layout & Extents
  auto set_align_h(int align) -> void;
  auto text_extent_w() const -> int;
  auto refresh_layout() -> int;
  auto set_visible(bool visible) -> int;

  // 4. Texture & Colors
  // SRO color (0xAABBGGRR) via CIFTextColor_SetAll (+0x90). Does not touch set_text_mode (+0x374).
  auto set_text_color(std::uint32_t sro_color) -> int;
  auto set_texture_path(const char* path) -> bool;

  // 5. Static Inspection & Factory
  static auto is_static(const cgwnd* wnd) -> bool;
  static auto static_label(cgwnd* wnd) -> cif_static*;
  static auto read_text(const cgwnd* wnd, wchar_t* dst, std::size_t dst_count) -> bool;
  static auto read_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool;
  static auto create_outer_wnd(cps_outer_interface* parent, void* res_descriptor, const cgwnd_create_rect& rect, int create_mode = 0, int user_flags = 0) -> cif_static*;
  static auto version_label_res() -> void*;
  static auto loading_banner_res() -> void*;
};
