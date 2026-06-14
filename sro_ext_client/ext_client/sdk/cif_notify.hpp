#pragma once
#include "cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

class cif_notify : public cif_static {
private:
  PAD_TO(ext_client::offsets::cif_static::size, ext_client::offsets::cif_notify::fields::static_text);
  cif_static* m_static_text;
  std::uint32_t m_duration;
  bool m_is_active;
  PAD_TO(ext_client::offsets::cif_notify::fields::is_active + sizeof(bool), ext_client::offsets::cif_notify::fields::y_position);
  std::int32_t m_y_position;
  PAD_TO(ext_client::offsets::cif_notify::fields::y_position + sizeof(std::int32_t), ext_client::offsets::cif_notify::size);

public:
  auto is_active() const -> bool;
  auto set_active(bool active) -> void;

  auto duration() const -> std::uint32_t;
  auto set_duration(std::uint32_t ms) -> void;

  auto y_position() const -> std::int32_t;
  auto set_y_position(std::int32_t y) -> void;

  auto static_text() -> cif_static*;

  auto set_background_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> void;
  auto show_message(const ext_client::msvc9::wstring& message) -> void;

  auto compile_time_layout_validation() -> void;
};
