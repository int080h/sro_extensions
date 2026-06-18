#include "cif_notify.hpp"

#include "utils/offsets.hpp"

auto cif_notify::is_active() const -> bool {
  return m_is_active;
}

auto cif_notify::set_active(bool active) -> void {
  m_is_active = active;
  static_vftable()->set_visible(this, active ? 1 : 0);
}

auto cif_notify::duration() const -> std::uint32_t {
  return m_duration;
}

auto cif_notify::set_duration(std::uint32_t ms) -> void {
  m_duration = ms;
}

auto cif_notify::y_position() const -> std::int32_t {
  return m_y_position;
}

auto cif_notify::set_y_position(std::int32_t y) -> void {
  m_y_position = y;
}

auto cif_notify::static_text() -> cif_static* {
  return m_static_text;
}

auto cif_notify::set_background_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> void {
  using set_color_fn = void(__thiscall*)(void*, std::uint8_t, std::uint8_t, std::uint8_t);
  ext_client::offsets::as_fn<set_color_fn>(ext_client::offsets::cif_notify::functions::set_bg_color)(this, r, g, b);
}

auto cif_notify::show_message(const ext_client::msvc9::wstring& message) -> void {
  using show_msg_fn = void(__thiscall*)(void*, const void*);
  ext_client::offsets::as_fn<show_msg_fn>(ext_client::offsets::cif_notify::functions::show_message)(this, message.raw());
}

#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
auto cif_notify::compile_time_layout_validation() -> void {
  static_assert(offsetof(cif_notify, m_static_text) == ext_client::offsets::cif_notify::fields::static_text, "m_static_text offset mismatch");
  static_assert(offsetof(cif_notify, m_duration) == ext_client::offsets::cif_notify::fields::duration, "m_duration offset mismatch");
  static_assert(offsetof(cif_notify, m_is_active) == ext_client::offsets::cif_notify::fields::is_active, "m_is_active offset mismatch");
  static_assert(offsetof(cif_notify, m_y_position) == ext_client::offsets::cif_notify::fields::y_position, "m_y_position offset mismatch");
  static_assert(sizeof(cif_notify) == ext_client::offsets::cif_notify::size, "cif_notify size mismatch");
}
#pragma warning(pop)
