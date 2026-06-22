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
  auto res_ui_root() -> ext_client::msvc9::n_map<int, void*>* { return &m_res_ui_root; }
  auto res_ui_root() const -> const ext_client::msvc9::n_map<int, void*>* { return &m_res_ui_root; }

  auto get_ui_child(int control_id, bool add_base_key = true) -> void*;

  // Find a child widget by res_id, trying res map first then get_child_by_unique_id.
  auto find_child(int res_id) -> cgwnd*;

  // Find the res map key for a widget within this outer's res map.
  auto res_map_key_for(const cgwnd* widget) -> int;

  auto login_phase() const -> int { return m_login_phase; }

  auto net_state() const -> int { return m_net_state; }

  auto set_net_state(int state) -> void { m_net_state = state; }

  auto load_thread() const -> void* { return m_load_thread; }
#pragma warning(pop)

using res_ui_root_map = std::n_map<int, void*>;

public:
  cps_outer_interface_vtable* vftable;
  union {
    DEFINE_MEMBER_N(int m_field_0c, 0x08);
    DEFINE_MEMBER_N(std::n_list<void*> m_obj_child_list, 0x10);
    DEFINE_MEMBER_N(int m_control_id, 0x28);
    DEFINE_MEMBER_N(cgwnd* m_parent, 0x2C);
    DEFINE_MEMBER_N(int m_create_flags, 0x30);
    DEFINE_MEMBER_N(std::uint8_t m_initialized, 0x38);
    DEFINE_MEMBER_N(int m_rect_x, 0x3C);
    DEFINE_MEMBER_N(int m_rect_y, 0x40);
    DEFINE_MEMBER_N(int m_rect_w, 0x44);
    DEFINE_MEMBER_N(int m_rect_h, 0x48);
    DEFINE_MEMBER_N(std::uint8_t m_visible, 0x5D);
    DEFINE_MEMBER_N(std::uint8_t m_gwnd_flag_62, 0x5E);
    DEFINE_MEMBER_N(std::uint8_t m_gwnd_flag_63, 0x5F);
    DEFINE_MEMBER_N(std::uint8_t m_gwnd_flag_64, 0x60);
    DEFINE_MEMBER_N(int m_user_data, 0x64);
    DEFINE_MEMBER_N(void* m_child_process_list, 0x68);
    DEFINE_MEMBER_N(void* m_wnd_child_list, 0x78);
    DEFINE_MEMBER_N(int m_net_state, 0x80);
    DEFINE_MEMBER_N(void* m_load_thread, 0x9C);
    DEFINE_MEMBER_N(res_ui_root_map m_res_ui_root, 0xAC);
    DEFINE_MEMBER_N(void* m_res_loader, 0xDC);
    DEFINE_MEMBER_N(int m_login_phase, 0xE0);
    DEFINE_MEMBER_N(int m_login_mode, 0xE4);
    DEFINE_MEMBER_N(void* m_gfx_child, 0xEC);
    DEFINE_MEMBER_N(int m_field_f4, 0xF0);
    DEFINE_MEMBER_N(int m_timer_value, 0xF4);
    DEFINE_MEMBER_N(int m_field_fc, 0xF8);
    DEFINE_MEMBER_N(int m_msg_list_head, 0xFC);
    DEFINE_MEMBER_N(void* m_msg_list_ptr, 0x100);
    DEFINE_MEMBER_N(int m_msg_list_count, 0x104);
  };

  static inline auto check_layout() -> void {
    
  }
};
