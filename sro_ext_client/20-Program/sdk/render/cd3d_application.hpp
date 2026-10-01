#pragma once

#include "sdk/types/cd3d_enumeration.hpp"
#include "sdk/types/d3d_adapter_info.hpp"
#include "sdk/types/d3d_device_info.hpp"

#include <cstddef>

#include <cstdint>

#include <d3d9.h>

class cd3d_application;



class cd3d_application {

public:

  auto is_initialized() const -> bool;
  auto is_windowed() const -> bool;
  auto is_active() const -> bool;
  auto is_ready() const -> bool;

  auto get_d3dpp() -> D3DPRESENT_PARAMETERS&;
  auto get_d3dpp() const -> const D3DPRESENT_PARAMETERS&;
  auto get_devmode() -> DEVMODEA&;
  auto get_devmode() const -> const DEVMODEA&;
  auto get_gamma_ramp() -> std::uint16_t*;
  auto get_gamma_ramp() const -> const std::uint16_t*;
  auto get_hwnd() const -> HWND;
  auto get_hwnd_focus() const -> HWND;
  auto get_hwnd_device() const -> HWND;
  auto get_d3d() const -> IDirect3D9*;
  auto get_device() const -> IDirect3DDevice9*;
  auto get_window_title() const -> const char*;
  auto get_creation_width() const -> std::uint32_t;
  auto get_creation_height() const -> std::uint32_t;
  auto get_window_rect() -> RECT&;
  auto get_window_rect() const -> const RECT&;
  auto get_client_rect() -> RECT&;
  auto get_client_rect() const -> const RECT&;
  auto get_render_target_width() -> std::uint32_t&;
  auto get_render_target_height() -> std::uint32_t&;
  auto get_display_frequency() -> std::uint32_t&;
  auto get_adapter_info_windowed() const -> d3d_adapter_info*;
  auto get_adapter_info_fullscreen() const -> d3d_adapter_info*;

protected:

private:

  auto is_active_flag() -> bool;

  auto get_enumeration() -> cd3d_enumeration;
  auto get_use_windowed_device() -> std::uint32_t;
  auto get_windowed_backbuffer_width() -> std::uint32_t;
  auto get_windowed_backbuffer_height() -> std::uint32_t;
  auto get_padapter_info_windowed() -> d3d_adapter_info*;
  auto get_padapter_info_fullscreen() -> d3d_adapter_info*;
  auto get_windowed() -> std::uint8_t;
  auto get_ready() -> std::uint8_t;
  auto get_has_focus() -> std::uint8_t;
  auto get_device_lost() -> std::uint8_t;
  auto get_minimized() -> std::uint8_t;
  auto get_rc_window() -> RECT;
  auto get_rc_client() -> RECT;
  auto get_is_initialized() -> std::uint32_t;

  auto set_active(bool val) -> void;
  auto set_enumeration(cd3d_enumeration val) -> void;
  auto set_use_windowed_device(std::uint32_t val) -> void;
  auto set_windowed_backbuffer_width(std::uint32_t val) -> void;
  auto set_windowed_backbuffer_height(std::uint32_t val) -> void;
  auto set_padapter_info_windowed(d3d_adapter_info* val) -> void;
  auto set_padapter_info_fullscreen(d3d_adapter_info* val) -> void;
  auto set_windowed(std::uint8_t val) -> void;
  auto set_ready(std::uint8_t val) -> void;
  auto set_has_focus(std::uint8_t val) -> void;
  auto set_device_lost(std::uint8_t val) -> void;
  auto set_minimized(std::uint8_t val) -> void;
  auto set_d3dpp(D3DPRESENT_PARAMETERS val) -> void;
  auto set_hwnd(HWND val) -> void;
  auto set_hwnd_focus(HWND val) -> void;
  auto set_hwnd_device(HWND val) -> void;
  auto set_d3d(IDirect3D9* val) -> void;
  auto set_device(IDirect3DDevice9* val) -> void;
  auto set_rc_window(RECT val) -> void;
  auto set_rc_client(RECT val) -> void;
  auto set_window_title(const char* val) -> void;
  auto set_creation_width(std::uint32_t val) -> void;
  auto set_creation_height(std::uint32_t val) -> void;
  auto set_is_initialized(std::uint32_t val) -> void;
  auto set_render_target_width(std::uint32_t val) -> void;
  auto set_render_target_height(std::uint32_t val) -> void;
  auto set_devmode(DEVMODEA val) -> void;

};





