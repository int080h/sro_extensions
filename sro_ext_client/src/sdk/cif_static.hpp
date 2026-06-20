#pragma once

#include "cgwnd.hpp"

#include <cstddef>
#include <cstdint>

namespace ext_client::msvc9 {
  class wstring;
}

class cif_static;

// CPSTitle bottom version labels use an 80px-wide rect; ellipsis expands on hover when enabled.
enum class cif_text_clip_mode {
  full,
  ellipsis_hover,
};

inline constexpr int k_default_ellipsis_clip_width = 80;

// +0x374 set_text_mode (NOT ARGB). CIFStatic ctor stores 1; passed to CIFStatic_SetTextCore.
inline constexpr int k_set_text_mode_ellipsis = 1;
inline constexpr int k_set_text_mode_plain = 0;

// SRO color block (0xAABBGGRR) applied before drawing static text (CIFTextColor_SetAll / +0x90).
struct cif_text_color_state {
  PAD_BYTES(pad_00, 0x10);
  std::uint32_t m_color_0;
  std::uint32_t m_color_1;
  std::uint32_t m_color_2;
  std::uint32_t m_color_3;
  std::uint32_t m_color_4;
};

struct cif_static_vtable {
  VFN_CDECL(get_static_res, void*);
  VFN_THISCALL(on_timer, void, cif_static* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, cif_static* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cif_static* self);
  VFN_THISCALL(equals, int, cif_static* self, cif_static* other);
  VFN_THISCALL(null_05, void, cif_static* self);
  VFN_CDECL(create_instance, cif_static*);
  VFN_THISCALL(on_create, int, cif_static* self, int mode);
  VFN_THISCALL(on_destroy, int, cif_static* self, int mode);
  VFN_THISCALL(on_init, int, cif_static* self, int mode);
  VFN_THISCALL(null_10, int, cif_static* self);
  VFN_THISCALL(null_11, int, cif_static* self);
  VFN_THISCALL(null_12, void, cif_static* self);
  VFN_THISCALL(on_cmd, int, cif_static* self, int cmd);
  VFN_THISCALL(on_render, int, cif_static* self, int pass);
  VFN_THISCALL(null_15, int, cif_static* self);
  VFN_THISCALL(run_update_chain, int, cif_static* self);
  VFN_THISCALL(on_update, int, cif_static* self);
  VFN_THISCALL(on_timer_id, int, cif_static* self, int timer_id);
  VFN_THISCALL(on_size, int, cif_static* self, int size_param);
  VFN_THISCALL(null_19, int, cif_static* self);
  VFN_THISCALL(on_move, int, cif_static* self, int move_param);
  VFN_THISCALL(on_show, int, cif_static* self, int show_param);
  // Misnamed: game slot 23 is CIFStatic_SetVisible (use cgwnd::set_visible, not this field).
  VFN_THISCALL(on_hide, int, cif_static* self, int hide_param);
  // Misnamed: game slot 24 is on_focus (sub_D5DB20), not set_visible.
  VFN_THISCALL(set_visible, int, cif_static* self, std::uint8_t visible);
  VFN_THISCALL(on_focus, int, cif_static* self, int focus_param);
  VFN_THISCALL(on_blur, int, cif_static* self, int blur_param);
  VFN_THISCALL(on_enable, int, cif_static* self, int enable_param);
  VFN_THISCALL(on_disable, int, cif_static* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, cif_static* self);
  VFN_THISCALL(null_38, int, cif_static* self);
  VFN_THISCALL(null_39, int, cif_static* self);
  VFN_THISCALL(null_40, int, cif_static* self);
  VFN_THISCALL(null_41, int, cif_static* self);
  VFN_THISCALL(update_text_layout, int, cif_static* self);
  VFN_THISCALL(null_43, int, cif_static* self);
  VFN_THISCALL(null_44, int, cif_static* self);
  VFN_THISCALL(set_text, char, cif_static* self, const wchar_t* text);
  VFN_THISCALL(null_46, int, cif_static* self);
  VFN_THISCALL(null_47, int, cif_static* self);
  VFN_THISCALL(null_48, int, cif_static* self);
  // NOTE: variadic — call only through cif_static_api wrappers.
  VFN_CDECL(set_text_fmt, char, cif_static* self, const wchar_t* fmt, ...);
  VFN_THISCALL(null_50, int, cif_static* self);
  VFN_THISCALL(null_51, int, cif_static* self);
  VFN_THISCALL(null_52, int, cif_static* self);
};

// CIFStatic: label / static text control (dword_1179970 resource).
class cif_static : public cgwnd {
public:
  DECLARE_SDK_WND_CAST(cif_static)
  DECLARE_SDK_VTABLE(cif_static_vtable, static_vftable)

  auto set_visible(bool visible) -> int;
  auto set_text(const wchar_t* text) -> char;
  auto set_text(const ext_client::msvc9::wstring& text) -> char;
  // SRO color (0xAABBGGRR) via CIFTextColor_SetAll (+0x90). Does not touch set_text_mode (+0x374).
  auto set_text_color(std::uint32_t sro_color) -> int;
  auto set_align_h(int align) -> void { m_align_h = align; }
  auto text_mode() const -> int;
  auto set_text_mode(int mode) -> void;
  auto refresh_layout() -> int;
  auto text_extent_w() const -> int;
  auto ellipsis_hover_enabled() const -> bool;
  auto set_text_clip_mode(cif_text_clip_mode mode, int ellipsis_width = k_default_ellipsis_clip_width) -> void;
  auto show_full_text_no_hover() -> void;
  auto enable_ellipsis_hover(int clip_width = k_default_ellipsis_clip_width) -> void;

  auto set_text_fmt(const wchar_t* fmt, int major, int minor) -> char;
  auto set_text_fmt(const wchar_t* fmt, double value) -> char;

  auto text(wchar_t* dst, std::size_t dst_count) const -> bool;

  // Check if a cgwnd is a CIFStatic or CIFDecoratedStatic by vtable.
  static auto is_static(const cgwnd* wnd) -> bool;
  // Cast a cgwnd to cif_static* if it's a static type, else nullptr.
  static auto as_if_static(cgwnd* wnd) -> cif_static*;
  // Read text from a widget that may be static or decorated static.
  static auto read_text(const cgwnd* wnd, wchar_t* dst, std::size_t dst_count) -> bool;
  // Read DDJ texture path from a static or decorated static widget.
  static auto read_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool;

  static auto create_outer_wnd(cgwnd* parent, void* res_descriptor, const cgwnd_create_rect& rect, int create_mode = 0, int user_flags = 0)
    -> cif_static*;
  static auto version_label_res() -> void*;

private:
  PAD_TO(ext_client::offsets::cgwnd::size, ext_client::offsets::cif_static::fields::align_h);
  int m_align_h;
  int m_align_v;
  cif_text_color_state m_text_color_state;
  PAD_TO(ext_client::offsets::cif_static::fields::text_color_state + sizeof(cif_text_color_state),
         ext_client::offsets::cif_static::fields::line_buffer);
  void* m_line_buffer_begin;
  void* m_line_buffer_end;
  PAD_TO(ext_client::offsets::cif_static::fields::line_buffer + sizeof(void*) * 2, ext_client::offsets::cif_static::fields::font_width);
  std::int16_t m_font_width;
  std::int16_t m_font_height;
  PAD_TO(ext_client::offsets::cif_static::fields::font_height + sizeof(std::int16_t),
         ext_client::offsets::cif_static::fields::set_text_mode);
  int m_set_text_mode;
  int m_text_flags;
  int m_saved_rect_w;
  PAD_TO(ext_client::offsets::cif_static::fields::saved_rect_w + sizeof(int), ext_client::offsets::cif_static::fields::text_bounds_x);
  float m_text_bounds_x;
  float m_text_bounds_y;
  float m_text_bounds_w;
  float m_text_bounds_h;
  PAD_TO(ext_client::offsets::cif_static::fields::text_bounds_h + sizeof(float), ext_client::offsets::cif_static::size);
};

static_assert(sizeof(cif_text_color_state) == 0x24, "cif_text_color_state size mismatch");
static_assert(sizeof(cif_static_vtable) == ext_client::offsets::cif_static::vtable::method_count * sizeof(void*),
              "cif_static_vtable method count mismatch");
static_assert(sizeof(cif_static) == ext_client::offsets::cif_static::size, "cif_static size mismatch");
