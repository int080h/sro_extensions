#include "hooks/sight_range_hook.hpp"
#include "sdk/sworld.hpp"
#include "config/client_config.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"

#include <algorithm>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::sight_range {
  namespace {

    make_hook<convention_type::thiscall_t, int, void*, unsigned int, int> g_set_graphics_option;
    hook_group g_hooks;

    struct sight_range_values {
      float range;
      std::int32_t cell_limit;
      float fade;
    };

    constexpr sight_range_values g_levels[] = {
      { 1500.0f, 800,  1200.0f }, // Level 0
      { 2500.0f, 1000, 2000.0f }, // Level 1
      { 3500.0f, 1200, 2800.0f }, // Level 2
      { 4500.0f, 1500, 3600.0f }, // Level 3
      { 5500.0f, 2048, 4400.0f }, // Level 4
      { 6500.0f, 2600, 4400.0f }, // Level 5
      { 7500.0f, 3200, 4400.0f }  // Level 6
    };

    auto __fastcall set_graphics_option_detour(void* self, void* edx, unsigned int setting_id, int value) -> int {
      // Pass the original value directly to the game's original function so options[8] gets a valid UI index.
      int result = g_set_graphics_option.call_original(self, setting_id, value);

      if (setting_id == 8) {
        apply_settings();
      }

      return result;
    }

  } // namespace

  auto apply_settings() -> void {
    auto* world = sworld::instance();
    if (!world || !sworld::is_instance()) {
      return;
    }

    const auto& cfg = ext_client::config::data().graphic;
    float range = 0.0f;
    std::int32_t cell_limit = 0;
    float fade = 0.0f;

    if (cfg.custom_sight_range) {
      const int percent = std::clamp(cfg.sight_range_percent, 20, 1000);

      const float factor = percent / 100.0f;
      range = 5500.0f * factor;
      cell_limit = static_cast<std::int32_t>(2048.0f * factor);
      fade = range * 0.8f;
    } else {
      int current_opt = world->option(8);

      int target_level = 0;
      // If the dropdown option is index 4 or 5, auto-upgrade it to Level 6 values.
      if (current_opt == 4 || current_opt == 5) {
        target_level = 6;
      } else {
        target_level = std::clamp(current_opt, 0, 6);
      }

      const auto& vals = g_levels[target_level];
      range = vals.range;
      cell_limit = vals.cell_limit;
      fade = vals.fade;
    }

    world->set_sight_range(range);
    world->set_cell_limit(cell_limit);
    ext_client::offsets::global_at<float>(ext_client::offsets::sworld::globals::details_fade_distance) = fade;
  }

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets::sworld::functions;
    if (!g_hooks.install(g_set_graphics_option, set_graphics_option, &set_graphics_option_detour, "sight_range_hook", "set_graphics_option")) {
      return false;
    }

    log_msg("[sight_range_hook] Hook installed successfully");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::sight_range
