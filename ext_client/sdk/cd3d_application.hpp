#pragma once

#include "utils/layout.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>
#include <d3d9.h>

class cd3d_application;

struct cd3d_enumeration_vtable {
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
};

struct cd3d_application_vtable {
  VFN_THISCALL(toggle_fullscreen, int, cd3d_application* self, int);
  VFN_THISCALL(set_menu, int, cd3d_application* self);
  VFN_THISCALL(build_device_list, void, cd3d_application* self);
  VFN_STDCALL(confirm_device, int, void* caps, void* format, void* windowed, void* multisample);
  VFN_THISCALL(one_time_scene_init, int, cd3d_application* self);
  VFN_THISCALL(init_device_objects, int, cd3d_application* self);
  VFN_THISCALL(restore_device_objects, int, cd3d_application* self);
  VFN_THISCALL(invalidate_device_objects, int, cd3d_application* self);
  VFN_THISCALL(delete_device_objects, int, cd3d_application* self);
  VFN_THISCALL(final_cleanup, int, cd3d_application* self);
  VFN_THISCALL(frame_move, int, cd3d_application* self);
  VFN_THISCALL(render, int, cd3d_application* self);
  VFN_THISCALL(create, int, cd3d_application* self, HINSTANCE instance);
  VFN_THISCALL(create_device, std::uintptr_t, cd3d_application* self);
  VFN_THISCALL(reset_3d_environment, int, cd3d_application* self, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  VFN_THISCALL(msg_proc, void, cd3d_application* self, char active);
  VFN_THISCALL(scalar_deleting_dtor, char*, cd3d_application* self, char should_free);
};

struct cd3d_enumeration {
  void* vftable;
  PAD(0x4);
  void* m_adapter_list;
  PAD(0x4);
  std::uint32_t m_min_width;
  std::uint32_t m_min_height;
  std::uint32_t m_min_backbuffer_format;
  std::uint32_t m_min_refresh;
  std::uint32_t m_depth_bits;
  std::uint32_t m_multisample_type;
  std::uint8_t m_can_do_windowed;
  std::uint8_t m_is_alpha;
  std::uint8_t m_is_stereo;
  PAD(0x1);
  void* m_device_combo;
};

struct d3d_adapter_info {
  void* vftable;
  PAD_TAIL(m_native, ext_client::offsets::d3d_adapter_info::size);
};

struct d3d_device_info {
  void* vftable;
  PAD_TAIL(m_native, ext_client::offsets::d3d_device_info::size);
};

class cd3d_application {
public:
  DECLARE_SDK_VTABLE(cd3d_application_vtable, app_vftable)

  auto d3dpp() -> D3DPRESENT_PARAMETERS&;
  auto d3dpp() const -> const D3DPRESENT_PARAMETERS&;

  auto devmode() -> DEVMODEA&;
  auto devmode() const -> const DEVMODEA&;

  auto gamma_ramp() -> std::uint16_t*;
  auto gamma_ramp() const -> const std::uint16_t*;

  auto hwnd() const -> HWND;
  auto hwnd_focus() const -> HWND;
  auto hwnd_device() const -> HWND;
  auto d3d() const -> IDirect3D9*;
  auto device() const -> IDirect3DDevice9* { return m_device; }

  auto windowed() const -> bool;
  auto active() const -> bool;
  auto ready() const -> bool;

  auto window_title() const -> const char*;
  auto creation_width() const -> std::uint32_t;
  auto creation_height() const -> std::uint32_t;
  auto is_initialized() const -> bool;

  auto window_rect() -> RECT&;
  auto window_rect() const -> const RECT&;
  auto client_rect() -> RECT&;
  auto client_rect() const -> const RECT&;

  auto render_target_width() -> std::uint32_t&;
  auto render_target_height() -> std::uint32_t&;
  auto display_frequency() -> std::uint32_t&;

  auto adapter_info_windowed() const -> d3d_adapter_info*;
  auto adapter_info_fullscreen() const -> d3d_adapter_info*;

protected:
  cd3d_application_vtable* vftable;

private:
  cd3d_enumeration m_enumeration;

  std::uint32_t m_use_windowed_device;
  std::uint32_t m_windowed_backbuffer_width;
  std::uint32_t m_windowed_backbuffer_height;
  d3d_adapter_info* m_padapter_info_windowed;

  PAD_TO(ext_client::offsets::cd3d_application::fields::padapter_info_windowed + sizeof(void*),
         ext_client::offsets::cd3d_application::fields::padapter_info_fullscreen);
  d3d_adapter_info* m_padapter_info_fullscreen;

  PAD_TO(ext_client::offsets::cd3d_application::fields::padapter_info_fullscreen + sizeof(void*),
         ext_client::offsets::cd3d_application::fields::b_windowed);
  std::uint8_t m_windowed;
  std::uint8_t m_active;
  std::uint8_t m_ready;
  std::uint8_t m_has_focus;
  std::uint8_t m_device_lost;
  std::uint8_t m_minimized;

  PAD_TO(ext_client::offsets::cd3d_application::fields::b_minimized + 1, ext_client::offsets::cd3d_application::fields::d3dpp);
  D3DPRESENT_PARAMETERS m_d3dpp;

  HWND m_hwnd;
  HWND m_hwnd_focus;
  HWND m_hwnd_device;
  IDirect3D9* m_d3d;
  IDirect3DDevice9* m_device;

  PAD_TO(ext_client::offsets::cd3d_application::fields::device + sizeof(void*), ext_client::offsets::cd3d_application::fields::rc_window);
  RECT m_rc_window;
  RECT m_rc_client;

  PAD_TO(ext_client::offsets::cd3d_application::fields::rc_client + sizeof(RECT),
         ext_client::offsets::cd3d_application::fields::window_title);
  const char* m_window_title;
  std::uint32_t m_creation_width;
  std::uint32_t m_creation_height;

  PAD_TO(ext_client::offsets::cd3d_application::fields::creation_height + sizeof(std::uint32_t),
         ext_client::offsets::cd3d_application::fields::is_initialized);
  std::uint32_t m_is_initialized;

  PAD_TO(ext_client::offsets::cd3d_application::fields::is_initialized + sizeof(std::uint32_t), 0x350);
  std::uint32_t m_render_target_width;
  std::uint32_t m_render_target_height;

  PAD_TO(0x350 + sizeof(std::uint32_t) * 2, ext_client::offsets::cd3d_application::fields::devmode);
  DEVMODEA m_devmode;

  PAD_TO(ext_client::offsets::cd3d_application::fields::devmode + sizeof(DEVMODEA),
         ext_client::offsets::cd3d_application::fields::gamma_ramp);
  std::uint16_t m_gamma_ramp[384];
};

static_assert(sizeof(cd3d_application) == ext_client::offsets::cd3d_application::size, "cd3d_application size mismatch");
