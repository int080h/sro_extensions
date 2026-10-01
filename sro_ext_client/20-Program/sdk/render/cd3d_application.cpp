#include "pch.hpp"
#include "sdk/render/cd3d_application.hpp"
#include "utils/offsets.hpp"

auto cd3d_application::get_d3dpp() -> D3DPRESENT_PARAMETERS& {
  return ext_client::off::field_at<D3DPRESENT_PARAMETERS>(this, 0x0AC);
}

auto cd3d_application::get_d3dpp() const -> const D3DPRESENT_PARAMETERS& {
  return ext_client::off::field_at<D3DPRESENT_PARAMETERS>(this, 0x0AC);
}

auto cd3d_application::get_devmode() -> DEVMODEA& {
  return ext_client::off::field_at<DEVMODEA>(this, 0x4B4);
}

auto cd3d_application::get_devmode() const -> const DEVMODEA& {
  return ext_client::off::field_at<DEVMODEA>(this, 0x4B4);
}

auto cd3d_application::get_gamma_ramp() -> std::uint16_t* {
  return &ext_client::off::field_at<std::uint16_t>(this, 0x590);
}

auto cd3d_application::get_gamma_ramp() const -> const std::uint16_t* {
  return &ext_client::off::field_at<std::uint16_t>(this, 0x590);
}

auto cd3d_application::get_hwnd() const -> HWND {
  return ext_client::off::field_at<HWND>(this, 0x0E4);
}

auto cd3d_application::get_hwnd_focus() const -> HWND {
  return ext_client::off::field_at<HWND>(this, 0x0E8);
}

auto cd3d_application::get_hwnd_device() const -> HWND {
  return ext_client::off::field_at<HWND>(this, 0x0EC);
}

auto cd3d_application::get_d3d() const -> IDirect3D9* {
  return ext_client::off::field_at<IDirect3D9*>(this, 0x0F0);
}

auto cd3d_application::get_device() const -> IDirect3DDevice9* {
  return ext_client::off::field_at<IDirect3DDevice9*>(this, 0x0F4);
}

auto cd3d_application::is_windowed() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x0A0) != 0;
}

auto cd3d_application::is_active() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x0A1) != 0;
}

auto cd3d_application::is_ready() const -> bool {
  return ext_client::off::field_at<std::uint8_t>(this, 0x0A2) != 0;
}

auto cd3d_application::get_window_title() const -> const char* {
  return ext_client::off::field_at<const char*>(this, 0x330);
}

auto cd3d_application::get_creation_width() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x334);
}

auto cd3d_application::get_creation_height() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x338);
}

auto cd3d_application::is_initialized() const -> bool {
  return ext_client::off::field_at<std::uint32_t>(this, 0x344) != 0;
}

auto cd3d_application::get_window_rect() -> RECT& {
  return ext_client::off::field_at<RECT>(this, 0x250);
}

auto cd3d_application::get_window_rect() const -> const RECT& {
  return ext_client::off::field_at<RECT>(this, 0x250);
}

auto cd3d_application::get_client_rect() -> RECT& {
  return ext_client::off::field_at<RECT>(this, 0x260);
}

auto cd3d_application::get_client_rect() const -> const RECT& {
  return ext_client::off::field_at<RECT>(this, 0x260);
}

auto cd3d_application::get_render_target_width() -> std::uint32_t& {
  return ext_client::off::field_at<std::uint32_t>(this, 0x350);
}

auto cd3d_application::get_render_target_height() -> std::uint32_t& {
  return ext_client::off::field_at<std::uint32_t>(this, 0x354);
}

auto cd3d_application::get_display_frequency() -> std::uint32_t& {
  return *reinterpret_cast<std::uint32_t*>(&ext_client::off::field_at<DEVMODEA>(this, 0x4B4).dmDisplayFrequency);
}

auto cd3d_application::get_adapter_info_windowed() const -> d3d_adapter_info* {
  return ext_client::off::field_at<d3d_adapter_info*>(this, 0x040);
}

auto cd3d_application::get_adapter_info_fullscreen() const -> d3d_adapter_info* {
  return ext_client::off::field_at<d3d_adapter_info*>(this, 0x078);
}
