#include "features/combat_overlay/combat_overlay.hpp"

#include "config/client_config.hpp"
#include "features/combat_overlay/combat_packets.hpp"
#include "features/combat_overlay/combat_target.hpp"
#include "features/combat_overlay/entity_manager.hpp"
#include "features/combat_overlay/target_window_hp.hpp"
#include "sdk/cicplayer.hpp"
#include "sdk/cif_target_window.hpp"

#include <Windows.h>

#include <deque>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ext_client::combat_overlay {
  namespace {

    struct damage_hit {
      DWORD tick = 0;
      std::uint32_t damage = 0;
    };

    struct fight_session {
      std::uint32_t target_uid = 0;
      std::wstring target_name;
      std::uint32_t damage_total_self = 0;
      std::uint32_t crit_count = 0;
      std::uint32_t hit_count = 0;
      DWORD fight_start_tick = 0;
      DWORD last_hit_tick = 0;
      std::deque<damage_hit> rolling;
      std::unordered_map<std::uint32_t, std::uint32_t> party_damage;
      std::unordered_map<std::uint32_t, std::wstring> name_cache;
    };

    fight_session g_session{};

    auto now_tick() -> DWORD {
      return GetTickCount();
    }

    auto fight_duration_sec() -> double {
      if (g_session.fight_start_tick == 0) {
        return 0.0;
      }
      const DWORD elapsed = now_tick() - g_session.fight_start_tick;
      return static_cast<double>(elapsed) / 1000.0;
    }

    auto prune_rolling(DWORD current_tick, DWORD window_ms) -> void {
      while (!g_session.rolling.empty() && current_tick - g_session.rolling.front().tick > window_ms) {
        g_session.rolling.pop_front();
      }
    }

    auto rolling_damage_sum() -> std::uint32_t {
      std::uint32_t sum = 0;
      for (const auto& hit : g_session.rolling) {
        sum += hit.damage;
      }
      return sum;
    }

    auto cache_name(std::uint32_t uid, const wchar_t* name) -> void {
      if (uid == 0 || !name || name[0] == L'\0') {
        return;
      }
      g_session.name_cache[uid] = name;
    }

    auto resolve_name(std::uint32_t uid) -> const wchar_t* {
      const auto it = g_session.name_cache.find(uid);
      if (it != g_session.name_cache.end()) {
        return it->second.c_str();
      }

      void* mgr = entity_manager::get();
      if (!mgr) {
        return L"???";
      }

      if (ci_charactor* entity = entity_manager::lookup_by_uid(mgr, uid)) {
        const wchar_t* name = entity_manager::display_name(entity);
        cache_name(uid, name);
        return name;
      }
      return L"???";
    }

    auto sync_target_from_selection() -> void {
      ci_charactor* target = combat_target::get_selected();
      if (!target) {
        return;
      }

      const std::uint32_t uid = target->unique_id();
      if (uid == 0) {
        return;
      }

      if (g_session.target_uid != uid) {
        const auto& cfg = ext_client::config::data().combat_overlay;
        if (cfg.reset_on_target_change && g_session.target_uid != 0) {
          reset_session();
        }
        g_session.target_uid = uid;
        g_session.target_name = target->display_name();
        cache_name(uid, g_session.target_name.c_str());
        if (g_session.fight_start_tick == 0) {
          g_session.fight_start_tick = now_tick();
        }
      } else if (g_session.target_name.empty()) {
        g_session.target_name = target->display_name();
      }
    }

    auto check_timeout() -> void {
      const auto& cfg = ext_client::config::data().combat_overlay;
      if (g_session.last_hit_tick == 0 || cfg.combat_timeout_sec <= 0) {
        return;
      }

      const DWORD elapsed = now_tick() - g_session.last_hit_tick;
      if (elapsed > static_cast<DWORD>(cfg.combat_timeout_sec) * 1000u) {
        reset_session();
      }
    }

    auto check_target_death() -> void {
      const auto& cfg = ext_client::config::data().combat_overlay;
      if (!cfg.reset_on_target_death || g_session.target_uid == 0) {
        return;
      }

      void* mgr = entity_manager::get();
      if (!mgr) {
        return;
      }

      ci_charactor* target = entity_manager::lookup_by_uid(mgr, g_session.target_uid);
      if (!target) {
        reset_session();
        return;
      }

      if (target->hp() == 0 || target->max_hp() == 0) {
        reset_session();
      }
    }

  } // namespace

  auto reset_session() -> void {
    g_session = fight_session{};
    cif_target_window::clear_active_panel();
  }

  auto on_damage_event(std::uint32_t attacker_uid,
                       std::uint32_t target_uid,
                       std::uint32_t damage,
                       std::uint8_t damage_flag,
                       std::uint32_t effect_flag) -> void {
    const auto& cfg = ext_client::config::data().combat_overlay;
    if (!cfg.enabled || damage == 0) {
      return;
    }

    if ((effect_flag & combat_packets::skill_effect_flag::Dead) != 0 && cfg.reset_on_target_death && target_uid == g_session.target_uid) {
      reset_session();
      return;
    }

    sync_target_from_selection();

    const DWORD tick = now_tick();
    if (g_session.fight_start_tick == 0) {
      g_session.fight_start_tick = tick;
    }
    g_session.last_hit_tick = tick;

    if (g_session.target_uid == 0 && target_uid != 0) {
      g_session.target_uid = target_uid;
      g_session.target_name = resolve_name(target_uid);
    }

    cache_name(attacker_uid, resolve_name(attacker_uid));
    cache_name(target_uid, resolve_name(target_uid));

    auto* player = cic_player::local();
    const std::uint32_t local_uid = player ? player->unique_id() : 0;
    if (attacker_uid == local_uid) {
      g_session.damage_total_self += damage;
      g_session.hit_count += 1;
      if ((damage_flag & combat_packets::skill_damage_flag::Critical) != 0) {
        g_session.crit_count += 1;
      }
      g_session.rolling.push_back(damage_hit{tick, damage});
      prune_rolling(tick, 3000);
    }

    if (cfg.show_party_dps && attacker_uid != 0 && attacker_uid != local_uid) {
      g_session.party_damage[attacker_uid] += damage;
    }
  }

  auto tick() -> void {
    const auto& cfg = ext_client::config::data().combat_overlay;
    if (!cfg.enabled) {
      return;
    }

    sync_target_from_selection();
    check_timeout();
    check_target_death();
  }

  auto wants_render() -> bool {
    const auto& cfg = ext_client::config::data().combat_overlay;
    return cfg.enabled && cfg.show_dps;
  }

  auto get_dps_stats() -> dps_stats {
    dps_stats stats{};
    stats.damage_total_self = g_session.damage_total_self;
    stats.crit_count = g_session.crit_count;
    stats.hit_count = g_session.hit_count;
    stats.target_name = g_session.target_name;
    stats.fight_duration_sec = fight_duration_sec();

    const DWORD tick = now_tick();
    prune_rolling(tick, 3000);
    const double rolling_window = 3.0;
    stats.dps_rolling = static_cast<double>(rolling_damage_sum()) / rolling_window;

    if (stats.fight_duration_sec > 0.0) {
      stats.dps_total = static_cast<double>(stats.damage_total_self) / stats.fight_duration_sec;
    }

    stats.party_rows.reserve(g_session.party_damage.size());
    for (const auto& entry : g_session.party_damage) {
      stats.party_rows.emplace_back(std::wstring(resolve_name(entry.first)), entry.second);
    }

    return stats;
  }

} // namespace ext_client::combat_overlay
