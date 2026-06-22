#pragma once
#include "cif_static.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

class cif_notify : public cif_static {
private:
  union {
    DEFINE_MEMBER_N(cif_static* m_static_text, 0x434);
    DEFINE_MEMBER_N(std::uint32_t m_duration, 0x438);
    DEFINE_MEMBER_N(bool m_is_active, 0x43C);
    DEFINE_MEMBER_N(std::int32_t m_y_position, 0x444);
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cif_notify::size - sizeof(cif_static)], "pad_end");
  };

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
