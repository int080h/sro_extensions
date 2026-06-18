#pragma once

#include <algorithm>
#include <cstdint>

namespace ext_client::utils::sight_range {

  struct values {
    float range = 0.0f;
    std::int32_t cell_limit = 0;
    float fade = 0.0f;
  };

  inline constexpr float k_base_range = 5500.0f;
  inline constexpr float k_base_cell = 2048.0f;

  inline constexpr values k_levels[] = {
    {1500.0f, 800, 1200.0f},
    {2500.0f, 1000, 2000.0f},
    {3500.0f, 1200, 2800.0f},
    {4500.0f, 1500, 3600.0f},
    {5500.0f, 2048, 4400.0f},
    {6500.0f, 2600, 4400.0f},
    {7500.0f, 3200, 4400.0f},
  };

  inline auto compute_from_percent(int percent) -> values {
    const int clamped = std::clamp(percent, 20, 1000);
    const float factor = clamped / 100.0f;
    const float range = k_base_range * factor;
    return {range, static_cast<std::int32_t>(k_base_cell * factor), range * 0.8f};
  }

  inline auto compute_from_level(int level) -> values {
    const int index = std::clamp(level, 0, static_cast<int>(sizeof(k_levels) / sizeof(k_levels[0])) - 1);
    return k_levels[index];
  }

} // namespace ext_client::utils::sight_range
