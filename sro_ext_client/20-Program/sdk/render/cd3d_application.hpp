#pragma once

#include "sdk/types/d3d_adapter_info.hpp"

#include <cstdint>
#include <windows.h>
#include <d3d9.h>

// ---------------------------------------------------------------------------
// CD3DApplication — Direct3D 9 Application Shell & Render Context Base
// Manages Direct3D device, focus HWNDs, display mode, and back buffers.
// ---------------------------------------------------------------------------
class cd3d_application {
public:
  // 1. Direct3D Context & Windows
  auto get_hwnd() const -> HWND;
  auto get_hwnd_focus() const -> HWND;
  auto get_hwnd_device() const -> HWND;
  auto get_d3d() const -> IDirect3D9*;
  auto get_device() const -> IDirect3DDevice9*;

  // 2. Viewport & Dimensions
  auto get_window_rect() -> RECT&;
  auto get_window_rect() const -> const RECT&;
  auto get_client_rect() -> RECT&;
  auto get_client_rect() const -> const RECT&;
  auto get_creation_width() const -> std::uint32_t;
  auto get_creation_height() const -> std::uint32_t;
  auto get_render_target_width() -> std::uint32_t&;
  auto get_render_target_height() -> std::uint32_t&;

  // 3. Engine State & Presentation Parameters
  auto get_d3dpp() -> D3DPRESENT_PARAMETERS&;
  auto get_d3dpp() const -> const D3DPRESENT_PARAMETERS&;
  auto get_devmode() -> DEVMODEA&;
  auto get_devmode() const -> const DEVMODEA&;
  auto get_gamma_ramp() -> std::uint16_t*;
  auto get_gamma_ramp() const -> const std::uint16_t*;
  auto is_windowed() const -> bool;
  auto is_active() const -> bool;
  auto is_ready() const -> bool;
  auto is_initialized() const -> bool;
  auto get_window_title() const -> const char*;
  auto get_display_frequency() -> std::uint32_t&;

  // 4. Adapter Information
  auto get_adapter_info_windowed() const -> d3d_adapter_info*;
  auto get_adapter_info_fullscreen() const -> d3d_adapter_info*;
};
