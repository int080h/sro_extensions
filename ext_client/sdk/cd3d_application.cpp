#include "cd3d_application.hpp"

auto cd3d_application::d3dpp() -> D3DPRESENT_PARAMETERS& {
  return m_d3dpp;
}

auto cd3d_application::d3dpp() const -> const D3DPRESENT_PARAMETERS& {
  return m_d3dpp;
}

auto cd3d_application::devmode() -> DEVMODEA& {
  return m_devmode;
}

auto cd3d_application::devmode() const -> const DEVMODEA& {
  return m_devmode;
}

auto cd3d_application::gamma_ramp() -> std::uint16_t* {
  return m_gamma_ramp;
}

auto cd3d_application::gamma_ramp() const -> const std::uint16_t* {
  return m_gamma_ramp;
}

auto cd3d_application::hwnd() const -> HWND {
  return m_hwnd;
}

auto cd3d_application::hwnd_focus() const -> HWND {
  return m_hwnd_focus;
}

auto cd3d_application::hwnd_device() const -> HWND {
  return m_hwnd_device;
}

auto cd3d_application::d3d() const -> IDirect3D9* {
  return m_d3d;
}

auto cd3d_application::windowed() const -> bool {
  return m_windowed != 0;
}

auto cd3d_application::active() const -> bool {
  return m_active != 0;
}

auto cd3d_application::ready() const -> bool {
  return m_ready != 0;
}

auto cd3d_application::window_title() const -> const char* {
  return m_window_title;
}

auto cd3d_application::creation_width() const -> std::uint32_t {
  return m_creation_width;
}

auto cd3d_application::creation_height() const -> std::uint32_t {
  return m_creation_height;
}

auto cd3d_application::is_initialized() const -> bool {
  return m_is_initialized != 0;
}

auto cd3d_application::window_rect() -> RECT& {
  return m_rc_window;
}

auto cd3d_application::window_rect() const -> const RECT& {
  return m_rc_window;
}

auto cd3d_application::client_rect() -> RECT& {
  return m_rc_client;
}

auto cd3d_application::client_rect() const -> const RECT& {
  return m_rc_client;
}

auto cd3d_application::render_target_width() -> std::uint32_t& {
  return m_render_target_width;
}

auto cd3d_application::render_target_height() -> std::uint32_t& {
  return m_render_target_height;
}

auto cd3d_application::display_frequency() -> std::uint32_t& {
  return *reinterpret_cast<std::uint32_t*>(&m_devmode.dmDisplayFrequency);
}

auto cd3d_application::adapter_info_windowed() const -> d3d_adapter_info* {
  return m_padapter_info_windowed;
}

auto cd3d_application::adapter_info_fullscreen() const -> d3d_adapter_info* {
  return m_padapter_info_fullscreen;
}
