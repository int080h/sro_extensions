#include "ext_client.hpp"

#include "hooks/sight_range_hook.hpp"
#include "sdk/sworld.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"
#include "utils/sight_range_math.hpp"

#include <algorithm>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;
using ext_client::utils::sight_range::compute_from_level;
using ext_client::utils::sight_range::compute_from_percent;

namespace ext_client::hooks::sight_range {
  namespace {

    make_hook<convention_type::thiscall_t, int, void*, unsigned int, int> g_set_graphics_option;
    hook_group g_hooks;

    auto __fastcall set_graphics_option_detour(void* self, void* edx, unsigned int setting_id, int value) -> int {
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
    ext_client::utils::sight_range::values vals{};

    if (cfg.custom_sight_range) {
      vals = compute_from_percent(cfg.sight_range_percent);
    } else {
      int current_opt = world->option(8);

      int target_level = 0;
      if (current_opt == 4 || current_opt == 5) {
        target_level = 6;
      } else {
        target_level = std::clamp(current_opt, 0, 6);
      }

      vals = compute_from_level(target_level);
    }

    world->set_sight_range(vals.range);
    world->set_cell_limit(vals.cell_limit);
    ext_client::offsets::global_at<float>(ext_client::offsets::sworld::globals::details_fade_distance) = vals.fade;
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
