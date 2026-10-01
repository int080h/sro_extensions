#include "pch.hpp"
#include "sdk/ui/cif_notify.hpp"

#include "utils/offsets.hpp"

auto cif_notify::is_active() const -> bool {
  return ext_client::off::field_at<bool>(this, 0x7D4);
}

auto cif_notify::set_active(bool active) -> void {
  ext_client::off::field_at<bool>(this, 0x7D4) = active;
  cgwnd::set_visible(this, active);
}

auto cif_notify::get_duration() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x7D0);
}

auto cif_notify::set_duration(std::uint32_t ms) -> void {
  ext_client::off::field_at<std::uint32_t>(this, 0x7D0) = ms;
}

auto cif_notify::get_y_position() const -> std::int32_t {
  return ext_client::off::field_at<std::int32_t>(this, 0x7DC);
}

auto cif_notify::set_y_position(std::int32_t y) -> void {
  ext_client::off::field_at<std::int32_t>(this, 0x7DC) = y;
}

auto cif_notify::get_static_text() -> cif_static* {
  return ext_client::off::field_at<cif_static*>(this, 0x7CC);
}

auto cif_notify::set_background_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> void {
  using set_color_fn = void(__thiscall*)(void*, std::uint8_t, std::uint8_t, std::uint8_t);
  ext_client::off::as_fn<set_color_fn>(0x008A0BA0)(this, r, g, b);
}

auto cif_notify::show_message(const ext_client::msvc9::wstring& message) -> void {
  using show_msg_fn = void(__thiscall*)(void*, const void*);
  ext_client::off::as_fn<show_msg_fn>(0x008A0D40)(this, message.raw());
}
