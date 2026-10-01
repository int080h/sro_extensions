#pragma once

#include <cstdint>

struct s_local_time {
  std::uint32_t dwRealTime;
  std::uint16_t wDay;
  std::uint8_t byHour;
  std::uint8_t byMin;
};
