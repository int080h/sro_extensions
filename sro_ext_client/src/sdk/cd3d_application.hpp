#pragma once

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
  union {
    DEFINE_MEMBER_N(void* m_adapter_list, ext_client::offsets::cd3d_enumeration::fields::adapter_list - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_min_width, ext_client::offsets::cd3d_enumeration::fields::min_width - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_min_height, ext_client::offsets::cd3d_enumeration::fields::min_height - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_min_backbuffer_format, ext_client::offsets::cd3d_enumeration::fields::min_format - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_min_refresh, ext_client::offsets::cd3d_enumeration::fields::min_refresh - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_depth_bits, ext_client::offsets::cd3d_enumeration::fields::depth_bits - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_multisample_type, ext_client::offsets::cd3d_enumeration::fields::multisample - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_can_do_windowed, ext_client::offsets::cd3d_enumeration::fields::can_windowed - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_is_alpha, ext_client::offsets::cd3d_enumeration::fields::is_alpha - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_is_stereo, ext_client::offsets::cd3d_enumeration::fields::is_stereo - sizeof(void*));
    DEFINE_MEMBER_N(void* m_device_combo, ext_client::offsets::cd3d_enumeration::fields::device_combo - sizeof(void*));
  };
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
  union {
    DEFINE_MEMBER_0(cd3d_enumeration m_enumeration, "enumeration");
    DEFINE_MEMBER_N(std::uint32_t m_use_windowed_device, ext_client::offsets::cd3d_application::fields::use_windowed_device - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_windowed_backbuffer_width, ext_client::offsets::cd3d_application::fields::windowed_backbuffer_width - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_windowed_backbuffer_height, ext_client::offsets::cd3d_application::fields::windowed_backbuffer_height - sizeof(void*));
    DEFINE_MEMBER_N(d3d_adapter_info* m_padapter_info_windowed, ext_client::offsets::cd3d_application::fields::padapter_info_windowed - sizeof(void*));
    DEFINE_MEMBER_N(d3d_adapter_info* m_padapter_info_fullscreen, ext_client::offsets::cd3d_application::fields::padapter_info_fullscreen - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_windowed, ext_client::offsets::cd3d_application::fields::b_windowed - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_active, ext_client::offsets::cd3d_application::fields::b_active - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_ready, ext_client::offsets::cd3d_application::fields::b_ready - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_has_focus, ext_client::offsets::cd3d_application::fields::b_has_focus - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_device_lost, ext_client::offsets::cd3d_application::fields::b_device_lost - sizeof(void*));
    DEFINE_MEMBER_N(std::uint8_t m_minimized, ext_client::offsets::cd3d_application::fields::b_minimized - sizeof(void*));
    DEFINE_MEMBER_N(D3DPRESENT_PARAMETERS m_d3dpp, ext_client::offsets::cd3d_application::fields::d3dpp - sizeof(void*));
    DEFINE_MEMBER_N(HWND m_hwnd, ext_client::offsets::cd3d_application::fields::hwnd - sizeof(void*));
    DEFINE_MEMBER_N(HWND m_hwnd_focus, ext_client::offsets::cd3d_application::fields::hwnd_focus - sizeof(void*));
    DEFINE_MEMBER_N(HWND m_hwnd_device, ext_client::offsets::cd3d_application::fields::hwnd_device - sizeof(void*));
    DEFINE_MEMBER_N(IDirect3D9* m_d3d, ext_client::offsets::cd3d_application::fields::d3d - sizeof(void*));
    DEFINE_MEMBER_N(IDirect3DDevice9* m_device, ext_client::offsets::cd3d_application::fields::device - sizeof(void*));
    DEFINE_MEMBER_N(RECT m_rc_window, ext_client::offsets::cd3d_application::fields::rc_window - sizeof(void*));
    DEFINE_MEMBER_N(RECT m_rc_client, ext_client::offsets::cd3d_application::fields::rc_client - sizeof(void*));
    DEFINE_MEMBER_N(const char* m_window_title, ext_client::offsets::cd3d_application::fields::window_title - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_creation_width, ext_client::offsets::cd3d_application::fields::creation_width - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_creation_height, ext_client::offsets::cd3d_application::fields::creation_height - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_is_initialized, ext_client::offsets::cd3d_application::fields::is_initialized - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_render_target_width, 0x350 - sizeof(void*));
    DEFINE_MEMBER_N(std::uint32_t m_render_target_height, 0x354 - sizeof(void*));
    DEFINE_MEMBER_N(DEVMODEA m_devmode, ext_client::offsets::cd3d_application::fields::devmode - sizeof(void*));
    DEFINE_MEMBER_N(std::uint16_t m_gamma_ramp[384], ext_client::offsets::cd3d_application::fields::gamma_ramp - sizeof(void*));
  };
};

