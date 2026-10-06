#pragma once

#include <Windows.h>
#include <cstdint>

namespace ext_client::sdk {

  struct elapsed_logout_parts {
    std::uint32_t months = 0;
    std::uint32_t days = 0;
    std::uint32_t hours = 0;
    std::uint32_t minutes = 0;
  };

  struct packed_time {
    std::uint32_t raw = 0;

    constexpr packed_time() noexcept = default;
    constexpr explicit packed_time(std::uint32_t value) noexcept : raw(value) {}

    [[nodiscard]] constexpr auto second() const noexcept -> std::uint8_t {
      return static_cast<std::uint8_t>(raw & 0x3F);
    }

    [[nodiscard]] constexpr auto month() const noexcept -> std::uint8_t {
      return static_cast<std::uint8_t>((raw >> 6) & 0x0F);
    }

    [[nodiscard]] constexpr auto day() const noexcept -> std::uint8_t {
      return static_cast<std::uint8_t>((raw >> 10) & 0x1F);
    }

    [[nodiscard]] constexpr auto hour() const noexcept -> std::uint8_t {
      return static_cast<std::uint8_t>((raw >> 15) & 0x1F);
    }

    [[nodiscard]] constexpr auto minute() const noexcept -> std::uint8_t {
      return static_cast<std::uint8_t>((raw >> 20) & 0x3F);
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
      const auto m = month();
      const auto d = day();
      const auto h = hour();
      const auto min = minute();
      return m >= 1 && m <= 12 && d >= 1 && d <= 31 && h <= 23 && min <= 59;
    }
  };

  auto is_leap_year(int year) noexcept -> bool;
  auto days_in_month(int year, int month) noexcept -> int;
  auto compute_elapsed_logout(packed_time time, elapsed_logout_parts& out, const SYSTEMTIME* ref_now = nullptr) -> bool;

} // namespace ext_client::sdk
