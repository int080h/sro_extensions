#pragma once

#include "utils/offsets.hpp"
#include "utils/layout.hpp"

#include <cstdint>

struct cprocess;

struct cprocess_msg {
  void* vtable;
  std::uint32_t msg_id;
  std::uintptr_t param1;
  std::uintptr_t param2;
};

struct cprocess_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cprocess* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cprocess* self);
  VFN_THISCALL(equals, int, cprocess* self, cprocess* other);
  VFN_THISCALL(null_05, void, cprocess* self);
  VFN_CDECL(create_instance, cprocess*);
  VFN_THISCALL(on_create, int, cprocess* self, int mode);
  VFN_THISCALL(on_destroy, int, cprocess* self, int mode);
  VFN_THISCALL(on_init, int, cprocess* self, int mode);
  VFN_THISCALL(null_10, int, cprocess* self);
  VFN_THISCALL(null_11, int, cprocess* self);
  VFN_THISCALL(null_12, void, cprocess* self);
  VFN_THISCALL(on_cmd, int, cprocess* self, int cmd);
  VFN_THISCALL(on_render, int, cprocess* self, int pass);
  VFN_THISCALL(null_15, int, cprocess* self);
  VFN_THISCALL(run_update_chain, int, cprocess* self);
  VFN_THISCALL(on_update, int, cprocess* self);
  VFN_THISCALL(on_timer, int, cprocess* self, int timer_id);
  VFN_THISCALL(on_size, int, cprocess* self, int size_param);
  VFN_THISCALL(null_19, int, cprocess* self);
  VFN_THISCALL(on_move, int, cprocess* self, int move_param);
  VFN_THISCALL(on_show, int, cprocess* self, int show_param);
  VFN_THISCALL(on_hide, int, cprocess* self, int hide_param);
  VFN_THISCALL(set_visible, int, cprocess* self, std::uint8_t visible);
  VFN_THISCALL(on_focus, int, cprocess* self, int focus_param);
  VFN_THISCALL(on_blur, int, cprocess* self, int blur_param);
  VFN_THISCALL(on_enable, int, cprocess* self, int enable_param);
  VFN_THISCALL(on_disable, int, cprocess* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, cprocess* self);
  VFN_THISCALL(null_38, int, cprocess* self);
  VFN_THISCALL(null_39, int, cprocess* self);
};

// View of the CProcess slice inside cps_outer_interface (+0x84 .. +0xAF).
// Obtain via cps_outer_interface::as_cprocess() (pointer already at region_begin).
struct cprocess {
  int m_net_state;
  void* m_thread_list_head[3];
  int m_thread_list_size;
  PAD_TO(ext_client::offsets::cprocess::fields::thread_list_size + sizeof(int) - ext_client::offsets::cprocess::fields::region_begin,
         ext_client::offsets::cprocess::fields::msg_queue_head - ext_client::offsets::cprocess::fields::region_begin);
  void* m_msg_queue_head[3];
  int m_msg_queue_size;
  PAD_TO(ext_client::offsets::cprocess::fields::msg_queue_size + sizeof(int) - ext_client::offsets::cprocess::fields::region_begin,
         ext_client::offsets::cprocess::fields::region_end - ext_client::offsets::cprocess::fields::region_begin);
};
