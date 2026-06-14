#pragma once

#include "utils/offsets.hpp"
#include "utils/layout.hpp"

#include <cstdint>

class cgwnd;

#define CGWND_VTABLE_COMMON(self_type, on_timer_name) \
  VFN_THISCALL(scalar_deleting_dtor, void*, self_type* self, char should_free); \
  VFN_THISCALL(on_post_ctor, void, self_type* self); \
  VFN_THISCALL(equals, int, self_type* self, self_type* other); \
  VFN_THISCALL(null_05, void, self_type* self); \
  VFN_CDECL(create_instance, self_type*); \
  VFN_THISCALL(on_create, int, self_type* self, int mode); \
  VFN_THISCALL(on_destroy, int, self_type* self, int mode); \
  VFN_THISCALL(on_init, int, self_type* self, int mode); \
  VFN_THISCALL(null_10, int, self_type* self); \
  VFN_THISCALL(null_11, int, self_type* self); \
  VFN_THISCALL(null_12, void, self_type* self); \
  VFN_THISCALL(on_cmd, int, self_type* self, int cmd); \
  VFN_THISCALL(on_render, int, self_type* self, int pass); \
  VFN_THISCALL(null_15, int, self_type* self); \
  VFN_THISCALL(run_update_chain, int, self_type* self); \
  VFN_THISCALL(on_update, int, self_type* self); \
  VFN_THISCALL(on_timer_name, int, self_type* self, int timer_id); \
  VFN_THISCALL(on_size, int, self_type* self, int size_param); \
  VFN_THISCALL(null_19, int, self_type* self); \
  VFN_THISCALL(on_move, int, self_type* self, int move_param); \
  VFN_THISCALL(on_show, int, self_type* self, int show_param); \
  VFN_THISCALL(on_hide, int, self_type* self, int hide_param); \
  VFN_THISCALL(set_visible, int, self_type* self, std::uint8_t visible);

struct cgwnd_vtable {
  VFN_CDECL(get_gwnd_manager, void*);
  VFN_CDECL(get_gwnd_manager_alt, void*);
  CGWND_VTABLE_COMMON(cgwnd, on_timer)
  VFN_THISCALL(on_focus, int, cgwnd* self, int focus_param);
  VFN_THISCALL(on_blur, int, cgwnd* self, int blur_param);
  VFN_THISCALL(on_enable, int, cgwnd* self, int enable_param);
  VFN_THISCALL(on_disable, int, cgwnd* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_CDECL(null_37, void);
};

struct cgwnd_base_vtable {
  VFN_CDECL(get_interface_root, void*);
  VFN_CDECL(get_interface_root_alt, void*);
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, void* self);
  VFN_THISCALL(equals, int, void* self, void* other);
  VFN_THISCALL(null_05, void, void* self);
  VFN_CDECL(create_instance, void*);
  VFN_THISCALL(on_create, int, void* self, int mode);
  VFN_THISCALL(on_destroy, int, void* self, int mode);
  VFN_THISCALL(on_init, int, void* self, int mode);
  VFN_THISCALL(on_attach, int, void* self);
  VFN_THISCALL(on_detach, int, void* self);
  VFN_THISCALL(on_cmd, int, void* self, int cmd);
  VFN_THISCALL(null_14, int, void* self);
  VFN_THISCALL(null_15, int, void* self);
  VFN_THISCALL(null_16, int, void* self);
  VFN_THISCALL(on_update, int, void* self);
  VFN_THISCALL(on_timer, int, void* self, int timer_id);
  VFN_THISCALL(on_size, int, void* self, int size_param);
  VFN_THISCALL(null_19, int, void* self);
  VFN_THISCALL(on_move, int, void* self, int move_param);
  VFN_THISCALL(on_show, int, void* self, int show_param);
  VFN_THISCALL(on_hide, int, void* self, int hide_param);
  VFN_THISCALL(set_visible, int, void* self, std::uint8_t visible);
};

// CGWndBase::wnd_rect — absolute client bounds maintained by the game (GetBounds).
struct cgwnd_bounds {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

// Rectangle passed to CGWnd_CreateOuter (sub_D62410): type + position + size.
struct cgwnd_create_rect {
  int type;
  int x;
  int y;
  int width;
};

// Base UI window (GWnd.cpp). Size 0x84 bytes before derived IF controls.
class cgwnd {
public:
  DECLARE_SDK_VTABLE(cgwnd_vtable, wnd_vftable)

  auto set_visible(bool visible) -> int;
  auto set_position(int x, int y) -> int;
  auto set_size(int width, int height) -> int;
  auto set_anim(int alpha, float speed, float delay, int mode) -> void;

  auto parent() const -> cgwnd* { return m_parent; }
  auto rect_x() const -> int { return m_rect_x; }
  auto rect_y() const -> int { return m_rect_y; }
  auto rect_w() const -> int { return m_rect_w; }
  auto rect_h() const -> int { return m_rect_h; }
  auto set_rect_w(int width) -> void { m_rect_w = width; }
  auto control_id() const -> int { return m_control_id; }
  auto visible() const -> bool { return m_visible != 0; }
  auto unique_id() const -> int { return m_create_flags; }
  auto get_bounds() const -> cgwnd_bounds;

  static auto client_config() -> void*;
  static auto client_data_version() -> unsigned;
  static auto screen_height() -> int;
  static auto screen_width() -> int;
  static auto set_position(cgwnd* wnd, int x, int y) -> int;
  static auto set_size(cgwnd* wnd, int width, int height) -> int;
  static auto set_visible(cgwnd* wnd, bool visible) -> int;
  static auto interface_under_cursor() -> cgwnd*;

public:
  cgwnd_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cgwnd::fields::control_id);
  int m_control_id;
  cgwnd* m_parent;
  int m_create_flags;
  PAD_TO(ext_client::offsets::cgwnd::fields::create_flags + sizeof(int), ext_client::offsets::cgwnd::fields::initialized);
  std::uint8_t m_initialized;
  PAD_TO(ext_client::offsets::cgwnd::fields::initialized + sizeof(std::uint8_t), ext_client::offsets::cgwnd::fields::rect_x);
  int m_rect_x;
  int m_rect_y;
  int m_rect_w;
  int m_rect_h;
  PAD_TO(ext_client::offsets::cgwnd::fields::rect_h + sizeof(int), ext_client::offsets::cgwnd::fields::visible);
  std::uint8_t m_visible;
  PAD_TO(ext_client::offsets::cgwnd::fields::visible + sizeof(std::uint8_t), ext_client::offsets::cgwnd::fields::user_data);
  int m_user_data;
  void* m_heap_res_descriptor; // CPS screens use +0x6C as child_process_list instead
  PAD_TO(ext_client::offsets::cgwnd::fields::heap_res_descriptor + sizeof(void*), ext_client::offsets::cgwnd::fields::child_list);
  void* m_child_list;
  PAD_TO(ext_client::offsets::cgwnd::fields::child_list + sizeof(void*), ext_client::offsets::cgwnd::size);
};

static_assert(sizeof(cgwnd_vtable) == ext_client::offsets::cgwnd::vtable::method_count * sizeof(void*),
              "cgwnd_vtable method count mismatch");
static_assert(offsetof(cgwnd, m_control_id) == ext_client::offsets::cgwnd::fields::control_id, "cgwnd::m_control_id offset mismatch");
static_assert(offsetof(cgwnd, m_parent) == ext_client::offsets::cgwnd::fields::parent, "cgwnd::m_parent offset mismatch");
static_assert(offsetof(cgwnd, m_create_flags) == ext_client::offsets::cgwnd::fields::create_flags, "cgwnd::m_create_flags offset mismatch");
static_assert(offsetof(cgwnd, m_initialized) == ext_client::offsets::cgwnd::fields::initialized, "cgwnd::m_initialized offset mismatch");
static_assert(offsetof(cgwnd, m_rect_x) == ext_client::offsets::cgwnd::fields::rect_x, "cgwnd::m_rect_x offset mismatch");
static_assert(offsetof(cgwnd, m_rect_y) == ext_client::offsets::cgwnd::fields::rect_y, "cgwnd::m_rect_y offset mismatch");
static_assert(offsetof(cgwnd, m_rect_w) == ext_client::offsets::cgwnd::fields::rect_w, "cgwnd::m_rect_w offset mismatch");
static_assert(offsetof(cgwnd, m_rect_h) == ext_client::offsets::cgwnd::fields::rect_h, "cgwnd::m_rect_h offset mismatch");
static_assert(offsetof(cgwnd, m_visible) == ext_client::offsets::cgwnd::fields::visible, "cgwnd::m_visible offset mismatch");
static_assert(offsetof(cgwnd, m_user_data) == ext_client::offsets::cgwnd::fields::user_data, "cgwnd::m_user_data offset mismatch");
static_assert(offsetof(cgwnd, m_heap_res_descriptor) == ext_client::offsets::cgwnd::fields::heap_res_descriptor,
              "cgwnd::m_heap_res_descriptor offset mismatch");
static_assert(offsetof(cgwnd, m_child_list) == ext_client::offsets::cgwnd::fields::child_list, "cgwnd::m_child_list offset mismatch");
static_assert(sizeof(cgwnd) == ext_client::offsets::cgwnd::size, "cgwnd size mismatch");
