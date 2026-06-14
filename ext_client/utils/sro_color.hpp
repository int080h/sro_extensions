#pragma once

#include <cstdint>

// Silkroad client 32-bit colors use BGR channel order (not RGB / not ImGui IM_COL32).
//
// Dword layout: (A << 24) | (R << 16) | (G << 8) | B
// Memory bytes (LE): B, G, R, A
//
// Examples:
//   white             -> 0xFFFFFFFF
//   pure red (R=255)  -> 0xFFFF0000
//
// ImGui IM_COL32: (A << 24) | (B << 16) | (G << 8) | R — convert at UI boundaries.

namespace ext_client::utils::color {

  inline constexpr auto pack(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0xFF) -> std::uint32_t {
    return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) |
           static_cast<std::uint32_t>(b);
  }

  inline constexpr auto a(std::uint32_t sro_col) -> std::uint8_t {
    return static_cast<std::uint8_t>((sro_col >> 24) & 0xFFu);
  }

  inline constexpr auto r(std::uint32_t sro_col) -> std::uint8_t {
    return static_cast<std::uint8_t>((sro_col >> 16) & 0xFFu);
  }

  inline constexpr auto g(std::uint32_t sro_col) -> std::uint8_t {
    return static_cast<std::uint8_t>((sro_col >> 8) & 0xFFu);
  }

  inline constexpr auto b(std::uint32_t sro_col) -> std::uint8_t {
    return static_cast<std::uint8_t>(sro_col & 0xFFu);
  }

  inline constexpr auto from_imgui(std::uint32_t imgui_col) -> std::uint32_t {
    return pack(static_cast<std::uint8_t>(imgui_col & 0xFFu),
                static_cast<std::uint8_t>((imgui_col >> 8) & 0xFFu),
                static_cast<std::uint8_t>((imgui_col >> 16) & 0xFFu),
                static_cast<std::uint8_t>((imgui_col >> 24) & 0xFFu));
  }

  inline constexpr auto to_imgui(std::uint32_t sro_col) -> std::uint32_t {
    return (static_cast<std::uint32_t>(a(sro_col)) << 24) | (static_cast<std::uint32_t>(b(sro_col)) << 16) |
           (static_cast<std::uint32_t>(g(sro_col)) << 8) | static_cast<std::uint32_t>(r(sro_col));
  }

} // namespace ext_client::utils::color

namespace ext_client::utils {}
