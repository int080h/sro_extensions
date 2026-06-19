#pragma once

#include "cgwnd_base.hpp"
#include "cobj.hpp"
#include "cgwnd.hpp"
#include "cmsg_stream_buffer.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstdint>

struct cprocess;
struct cprocess_msg;
struct cps_outer_interface;
struct cps_silkroad;
struct cps_version_check;
struct cps_title;
struct cps_character_select;

struct cps_outer_interface_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cps_outer_interface* self, char should_free);
  VFN_CDECL(get_res_map, void*);
  VFN_THISCALL(gwnd_helper_04, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_timer, int, cps_outer_interface* self, int timer_id);
  VFN_THISCALL(on_msg_recv, int, cps_outer_interface* self, cmsg_stream_buffer* buffer);
  VFN_THISCALL(insert_child, int, cps_outer_interface* self, char a2);
  VFN_THISCALL(remove_child, int, cps_outer_interface* self, int child_id);
  VFN_THISCALL(remove_child_by_id, int, cps_outer_interface* self, int child_id);
  VFN_THISCALL(on_create, char, cps_outer_interface* self, int mode);
  VFN_THISCALL(on_active, char, cps_outer_interface* self);
  VFN_THISCALL(on_update, void, cps_outer_interface* self);
  VFN_CDECL(on_render_begin, int);
  VFN_CDECL(on_render, int, int a1, int a2);
  VFN_CDECL(on_render_end, void);
  VFN_THISCALL(run_process, int, cps_outer_interface* self);
  VFN_STDCALL(on_enter, char, cprocess_msg* msg);
  VFN_CDECL(on_leave, void);
  VFN_THISCALL(on_lost_focus, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_got_focus, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_size, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_move, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_activate, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_deactivate, int, cps_outer_interface* self, int a2);
  VFN_THISCALL(on_close, int, cps_outer_interface* self, int a2);
  VFN_CDECL(on_minimize, int);
  VFN_CDECL(on_restore, int, int a1);
  VFN_CDECL(on_maximize, int, int a1);
  VFN_CDECL(gwnd_mouse_move, int, int a1);
  VFN_CDECL(gwnd_mouse_down, int, int a1);
  VFN_CDECL(gwnd_mouse_up, int, int a1);
  VFN_CDECL(gwnd_mouse_wheel, int, int a1);
  VFN_CDECL(gwnd_key_down, int, int a1);
  VFN_CDECL(gwnd_key_up, int, int a1);
  VFN_CDECL(gwnd_char, int, int a1);
  VFN_CDECL(gwnd_ime, int, int a1);
  VFN_CDECL(null_37, void);
  VFN_CDECL(null_38, void);
  VFN_STDCALL(on_cmd, int, int a1);
  VFN_THISCALL(on_net_error, void, cps_outer_interface* self);
};

// Flat virtual-inheritance layout (all bases at mdisp 0). Derived types extend from +0x10C.
struct cps_outer_interface {
public:
  DECLARE_SDK_VTABLE(cps_outer_interface_vtable, outer_vftable)

  DECLARE_SDK_CAST(cobj, as_cobj)
  DECLARE_SDK_CAST(cobj_child, as_cobj_child)
  DECLARE_SDK_OFFSET_CAST(cgwnd_base, as_gwnd_base, ext_client::offsets::cgwnd_base::fields::region_begin)
  DECLARE_SDK_CAST(cgwnd, as_gwnd)
  DECLARE_SDK_OFFSET_CAST(cprocess, as_cprocess, ext_client::offsets::cprocess::fields::region_begin)
  DECLARE_SDK_OFFSET_CAST(cps_silkroad, as_silkroad, ext_client::offsets::cps_silkroad::fields::region_begin)

#pragma warning(push)
#pragma warning(disable : 4100) // Unreferenced formal parameter
  auto res_ui_root() -> void* { return reinterpret_cast<std::uint8_t*>(this) + ext_client::offsets::cps_silkroad::fields::res_ui_root; }

  auto get_ui_child(int control_id, bool add_base_key = true) -> void* {
    const auto* map = reinterpret_cast<const ext_client::msvc9::ui_res_map*>(res_ui_root());
    return map->find(control_id, add_base_key);
  }

  auto login_phase() const -> int { return m_login_phase; }

  auto net_state() const -> int { return m_net_state; }

  auto set_net_state(int state) -> void { m_net_state = state; }

  auto load_thread() const -> void* { return m_load_thread; }
#pragma warning(pop)

public:
  cps_outer_interface_vtable* vftable;
  PAD_TO(sizeof(void*), ext_client::offsets::cobj_child::fields::field_0c);
  int m_field_0c;
  PAD_TO(ext_client::offsets::cobj_child::fields::field_0c + sizeof(int), ext_client::offsets::cobj_child::fields::list_node);
  void* m_obj_child_list_node[3];
  PAD_TO(ext_client::offsets::cobj_child::fields::list_node + sizeof(void*) * 3, ext_client::offsets::cgwnd::fields::control_id);
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
  std::uint8_t m_gwnd_flag_62;
  std::uint8_t m_gwnd_flag_63;
  std::uint8_t m_gwnd_flag_64;
  PAD_TO(ext_client::offsets::cgwnd::fields::visible + 4, ext_client::offsets::cgwnd::fields::user_data);
  int m_user_data;
  void* m_child_process_list;
  PAD_TO(ext_client::offsets::cps_outer_interface::fields::child_process_list + sizeof(void*),
         ext_client::offsets::cps_outer_interface::fields::wnd_child_list);
  void* m_wnd_child_list;
  PAD_TO(ext_client::offsets::cps_outer_interface::fields::wnd_child_list + sizeof(void*),
         ext_client::offsets::cps_outer_interface::fields::net_state);
  int m_net_state;
  PAD_TO(ext_client::offsets::cps_outer_interface::fields::net_state + sizeof(int),
         ext_client::offsets::cps_outer_interface::fields::load_thread);
  void* m_load_thread;
  PAD_TO(ext_client::offsets::cps_outer_interface::fields::load_thread + sizeof(void*),
         ext_client::offsets::cps_silkroad::fields::res_ui_root);
  PAD_TO(ext_client::offsets::cps_silkroad::fields::res_ui_root, ext_client::offsets::cps_silkroad::fields::res_loader);
  void* m_res_loader;
  int m_login_phase;
  int m_login_mode;
  PAD_TO(ext_client::offsets::cps_silkroad::fields::login_mode + sizeof(int), ext_client::offsets::cps_outer_interface::fields::gfx_child);
  void* m_gfx_child;
  int m_field_f4;
  int m_timer_value;
  int m_field_fc;
  int m_msg_list_head;
  void* m_msg_list_ptr;
  int m_msg_list_count;

  static inline auto check_layout() -> void {
    static_assert(sizeof(cps_outer_interface) == ext_client::offsets::cps_outer_interface::derived_region_begin,
                  "cps_outer_interface layout mismatch");
  }
};
