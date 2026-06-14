#pragma once

#include "utils/offsets.hpp"
#include "utils/layout.hpp"

#include <cstdint>

struct cps_silkroad;

struct cps_silkroad_vtable {
  VFN_CDECL(get_factory_entry, void*);
  VFN_CDECL(get_process_id, int);
  VFN_THISCALL(scalar_deleting_dtor, void*, cps_silkroad* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, cps_silkroad* self);
  VFN_THISCALL(equals, int, cps_silkroad* self, cps_silkroad* other);
  VFN_THISCALL(null_05, void, cps_silkroad* self);
  VFN_CDECL(create_instance, cps_silkroad*);
  VFN_THISCALL(on_create, int, cps_silkroad* self, int mode);
  VFN_THISCALL(on_destroy, int, cps_silkroad* self, int mode);
  VFN_THISCALL(on_init, int, cps_silkroad* self, int mode);
  VFN_THISCALL(null_10, int, cps_silkroad* self);
  VFN_THISCALL(null_11, int, cps_silkroad* self);
  VFN_THISCALL(null_12, void, cps_silkroad* self);
  VFN_THISCALL(on_cmd, int, cps_silkroad* self, int cmd);
  VFN_THISCALL(on_render, int, cps_silkroad* self, int pass);
  VFN_THISCALL(null_15, int, cps_silkroad* self);
  VFN_THISCALL(run_update_chain, int, cps_silkroad* self);
  VFN_THISCALL(on_update, int, cps_silkroad* self);
  VFN_THISCALL(on_timer, int, cps_silkroad* self, int timer_id);
  VFN_THISCALL(on_size, int, cps_silkroad* self, int size_param);
  VFN_THISCALL(null_19, int, cps_silkroad* self);
  VFN_THISCALL(on_move, int, cps_silkroad* self, int move_param);
  VFN_THISCALL(on_show, int, cps_silkroad* self, int show_param);
  VFN_THISCALL(on_hide, int, cps_silkroad* self, int hide_param);
  VFN_THISCALL(set_visible, int, cps_silkroad* self, std::uint8_t visible);
  VFN_THISCALL(on_focus, int, cps_silkroad* self, int focus_param);
  VFN_THISCALL(on_blur, int, cps_silkroad* self, int blur_param);
  VFN_THISCALL(on_enable, int, cps_silkroad* self, int enable_param);
  VFN_THISCALL(on_disable, int, cps_silkroad* self, int disable_param);
  VFN_CDECL(on_mouse_move, int, int x, int y);
  VFN_CDECL(on_mouse_down, int, int button);
  VFN_CDECL(on_mouse_up, int, int button);
  VFN_CDECL(on_mouse_wheel, int, int delta);
  VFN_CDECL(on_key_down, int, int key);
  VFN_CDECL(on_key_up, int, int key);
  VFN_CDECL(on_char, int, int ch);
  VFN_CDECL(on_ime, int, int ime_param);
  VFN_THISCALL(null_37, int, cps_silkroad* self);
  VFN_THISCALL(null_38, int, cps_silkroad* self);
  VFN_THISCALL(null_39, int, cps_silkroad* self);
  VFN_THISCALL(null_40, int, cps_silkroad* self);
};

// View of the CPSilkroad slice (+0xB0 .. +0xEF).
// Obtain via cps_outer_interface::as_silkroad() (pointer already at region_begin).
struct cps_silkroad {
  PAD(ext_client::offsets::cps_silkroad::fields::res_loader - ext_client::offsets::cps_silkroad::fields::region_begin);
  void* m_res_loader;
  int m_login_phase;
  int m_login_mode;
  PAD_TO(ext_client::offsets::cps_silkroad::fields::login_mode + sizeof(int) - ext_client::offsets::cps_silkroad::fields::region_begin,
         ext_client::offsets::cps_silkroad::fields::region_end - ext_client::offsets::cps_silkroad::fields::region_begin);
};
