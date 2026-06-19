#include "ext_client.hpp"

#include "hooks/cnif_sro_ingame_start_hook.hpp"

#include <Windows.h>
#include <cmath>
#include <cstdint>

#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cprocess_manager.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"

using ext_client::utils::log_msg;

namespace ext_client::hooks {
  namespace {

    auto control_panel() -> cnif_sro_ingame_start_hook::control& {
      return cnif_sro_ingame_start_hook::control_panel();
    }

    // ---------------------------------------------------------------------------
    // Promo/Alarm state
    // ---------------------------------------------------------------------------
    unsigned g_config_mask = 0;
    unsigned g_applied_mask = 0;
    bool g_promo_saw_ingame = false;
    std::uint32_t g_promo_enforce_counter = 0;
    constexpr std::uint32_t k_promo_enforce_every_n_ticks = 60;

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------
    auto apply_promo_mask(cg_interface* iface, unsigned mask) -> void {
      if (!iface) {
        return;
      }

      const unsigned restored = g_applied_mask & ~mask;
      const unsigned newly_hidden = mask & ~g_applied_mask;

      if (restored != 0) {
        calarm_guide_mgr_wnd::apply_iface_promo_show(iface, static_cast<calarm_guide_mgr_wnd::promo_target>(restored));
      }

      if (newly_hidden != 0) {
        calarm_guide_mgr_wnd::apply_iface_promo_hide(iface, static_cast<calarm_guide_mgr_wnd::promo_target>(newly_hidden));
      } else if (mask != 0 && mask == g_applied_mask) {
        calarm_guide_mgr_wnd::apply_iface_promo_hide(iface, static_cast<calarm_guide_mgr_wnd::promo_target>(mask));
      }

      g_applied_mask = mask;
    }

  } // namespace

  auto cnif_sro_ingame_start_hook::control_panel() -> control& {
    return ext_client::config::data().interface_hide;
  }

  auto cnif_sro_ingame_start_hook::active_targets() -> calarm_guide_mgr_wnd::promo_target {
    using target = calarm_guide_mgr_wnd::promo_target;
    const auto& cfg = control_panel();
    unsigned mask = 0;
    if (cfg.hide_facebook) {
      mask |= static_cast<unsigned>(target::facebook);
    }
    if (cfg.hide_magic_lamp) {
      mask |= static_cast<unsigned>(target::magic_lamp);
    }
    if (cfg.hide_daily_login) {
      mask |= static_cast<unsigned>(target::daily_login);
    }
    if (cfg.hide_web_item_alarm) {
      mask |= static_cast<unsigned>(target::web_item_alarm);
    }
    if (cfg.hide_macro_guide) {
      mask |= static_cast<unsigned>(target::macro_guide);
    }
    return static_cast<target>(mask);
  }

  auto cnif_sro_ingame_start_hook::any_hide_active() -> bool {
    const auto& cfg = control_panel();
    return cfg.hide_facebook || cfg.hide_magic_lamp || cfg.hide_daily_login || cfg.hide_web_item_alarm || cfg.hide_macro_guide;
  }

  auto cnif_sro_ingame_start_hook::apply_promo_after_mgr_create(calarm_guide_mgr_wnd* mgr) -> void {
    if (!mgr || !calarm_guide_mgr_wnd::is_attached_to_iface(mgr)) {
      return;
    }

    const auto targets = active_targets();
    if (static_cast<unsigned>(targets) == 0) {
      return;
    }

    auto* iface = cg_interface::get();
    if (!iface || !cg_interface::is_instance(iface)) {
      return;
    }

    calarm_guide_mgr_wnd::apply_iface_promo_hide(iface, targets);
  }

  auto cnif_sro_ingame_start_hook::install() -> bool {
    return true;
  }

  auto cnif_sro_ingame_start_hook::uninstall() -> void {
  }

  auto cnif_sro_ingame_start_hook::is_installed() -> bool {
    return true;
  }

  auto cnif_sro_ingame_start_hook::apply_from_control() -> void {
    g_config_mask = static_cast<unsigned>(active_targets());

    if (cg_interface::is_ingame_hud_ready()) {
      if (auto* iface = cg_interface::get()) {
        apply_promo_mask(iface, g_config_mask);
      }
    }
  }

  auto cnif_sro_ingame_start_hook::tick() -> void {
    if (g_config_mask == 0) {
      return;
    }

    if (!cprocess_manager::is_ingame()) {
      calarm_guide_mgr_wnd::invalidate_cache();
      g_promo_saw_ingame = false;
      g_promo_enforce_counter = 0;
      return;
    }

    auto* iface = cg_interface::get();
    if (!iface) {
      return;
    }

    if (!g_promo_saw_ingame) {
      g_promo_saw_ingame = true;
      apply_promo_mask(iface, g_config_mask);
      g_promo_enforce_counter = 0;
    } else if (ext_client::utils::every_n_ticks(g_promo_enforce_counter, k_promo_enforce_every_n_ticks)) {
      calarm_guide_mgr_wnd::apply_iface_promo_hide(iface, static_cast<calarm_guide_mgr_wnd::promo_target>(g_config_mask));
      g_applied_mask = g_config_mask;
    }
  }

} // namespace ext_client::hooks
