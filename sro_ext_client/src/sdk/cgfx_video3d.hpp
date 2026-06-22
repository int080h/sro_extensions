#pragma once

#include "cd3d_application.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

class cgfx_video3d;

struct cgfx_video3d_vtable {
  VFN_THISCALL(toggle_fullscreen, int, cgfx_video3d* self, int);
  VFN_THISCALL(set_menu, int, cgfx_video3d* self);
  VFN_THISCALL(build_device_list, void, cgfx_video3d* self);
  VFN_THISCALL(confirm_device, int, cgfx_video3d* self, void* caps, char windowed, int format, int multisample);
  VFN_THISCALL(one_time_scene_init, int, cgfx_video3d* self);
  VFN_THISCALL(init_device_objects, int, cgfx_video3d* self);
  VFN_THISCALL(restore_device_objects, int, cgfx_video3d* self);
  VFN_THISCALL(invalidate_device_objects, int, cgfx_video3d* self);
  VFN_THISCALL(delete_device_objects, int, cgfx_video3d* self);
  VFN_THISCALL(final_cleanup, int, cgfx_video3d* self);
  VFN_THISCALL(frame_move, int, cgfx_video3d* self);
  VFN_THISCALL(render, int, cgfx_video3d* self);
  VFN_THISCALL(create, int, cgfx_video3d* self, HINSTANCE instance);
  VFN_THISCALL(create_device, std::uintptr_t, cgfx_video3d* self);
  VFN_THISCALL(reset_3d_environment, int, cgfx_video3d* self, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  VFN_THISCALL(msg_proc, void, cgfx_video3d* self, char active);
  VFN_THISCALL(scalar_deleting_dtor, int*, cgfx_video3d* self, char should_free);
  VFN_THISCALL(create_things, bool, cgfx_video3d* self, HWND hwnd, void* msg_handler, int flags);
  VFN_CDECL(destroy_things, char);
  VFN_THISCALL(init_render, std::uint32_t*, cgfx_video3d* self, std::uint32_t* out);
  VFN_THISCALL(set_size, bool, cgfx_video3d* self, std::uint32_t width, std::uint32_t height);
  VFN_THISCALL(get_size, std::uint32_t*, cgfx_video3d* self, std::uint32_t* out);
  VFN_THISCALL(get_backbuffer_format, std::uint32_t*, cgfx_video3d* self, std::uint32_t* out);
  VFN_THISCALL(set_gamma, int, cgfx_video3d* self, int ramp);
  VFN_THISCALL(begin_scene, char*, cgfx_video3d* self);
  VFN_THISCALL(end_scene_pre, char, cgfx_video3d* self);
  VFN_THISCALL(end_scene, bool, cgfx_video3d* self);
  VFN_THISCALL(present, bool, cgfx_video3d* self, int a2, int a3, int a4, int a5);
  VFN_THISCALL(set_format, int, cgfx_video3d* self, int format);
  VFN_THISCALL(get_format, int, cgfx_video3d* self);
  VFN_THISCALL(set_fov, int, cgfx_video3d* self);
  VFN_THISCALL(set_clip_planes, bool, cgfx_video3d* self, int mode);
  VFN_THISCALL(set_light, bool, cgfx_video3d* self, float index, int enabled, float* data);
  VFN_THISCALL(set_material, int, cgfx_video3d* self, char a2, int a3, int a4, int a5, int a6);
  VFN_THISCALL(set_view_matrix, bool, cgfx_video3d* self, float* matrix, float scale);
  VFN_THISCALL(set_proj_matrix, int, cgfx_video3d* self, int a2, int a3);
  VFN_THISCALL(set_world_matrix, char*, cgfx_video3d* self);
};

class cgfx_video3d : public cd3d_application {
public:
  DECLARE_SDK_VTABLE(cgfx_video3d_vtable, gfx_vftable)

  static auto get() -> cgfx_video3d*;

  auto create_things(HWND hwnd_param, void* handler, int flags) -> bool;
  auto destroy_things() -> bool;
  auto set_size(std::uint32_t width, std::uint32_t height) -> bool;
  auto begin_scene() -> bool;
  auto end_scene() -> bool;
  auto present(int a2 = 0, int a3 = 0, int a4 = 0, int a5 = 0) -> bool;
  auto set_format(int format) -> int;
  auto render() -> int;
  auto frame_move() -> int;
};
