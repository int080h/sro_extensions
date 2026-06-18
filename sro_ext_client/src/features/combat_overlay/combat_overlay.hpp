#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ext_client::combat_overlay {

  auto tick() -> void;
  auto render_imgui() -> void;
  auto wants_render() -> bool;
  auto reset_session() -> void;

  auto on_damage_event(std::uint32_t attacker_uid,
                       std::uint32_t target_uid,
                       std::uint32_t damage,
                       std::uint8_t damage_flag,
                       std::uint32_t effect_flag) -> void;

  struct dps_stats {
    std::uint32_t damage_total_self = 0;
    std::uint32_t crit_count = 0;
    std::uint32_t hit_count = 0;
    double fight_duration_sec = 0.0;
    double dps_rolling = 0.0;
    double dps_total = 0.0;
    std::wstring target_name;
    std::vector<std::pair<std::wstring, std::uint32_t>> party_rows;
  };

  auto get_dps_stats() -> dps_stats;

} // namespace ext_client::combat_overlay
