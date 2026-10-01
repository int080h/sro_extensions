#include "pch.hpp"
#include "plugins/hud_customizer/promo_hide.hpp"

#include "core/core_config.hpp"
#include "core/core_plugin_manager.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/calram_guide_mgr_wnd.hpp"

namespace ext_client::plugins::hud_customizer {

namespace {
  unsigned s_last_applied_mask = 0;
  bool s_has_applied = false;
}

auto reset_config_promo_hides_cache() -> void {
  s_last_applied_mask = 0;
  s_has_applied = false;
}

auto apply_config_promo_hides() -> void {
  if (!ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
    return;
  }

  const auto& cfg = ext_client::core::config::data().interface_hide;

  using target = calram_guide_mgr_wnd::promo_target;
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

  if (!s_has_applied) {
    s_has_applied = true;
    s_last_applied_mask = mask;
    if (mask != 0) {
      if (auto* iface = cg_interface::get()) {
        calram_guide_mgr_wnd::apply_iface_promo_hide(iface, static_cast<target>(mask));
      } else if (auto* mgr = calram_guide_mgr_wnd::get_current()) {
        mgr->apply_promo_hide(static_cast<target>(mask));
      }
    }
    return;
  }

  if (mask == s_last_applied_mask) {
    return;
  }

  const unsigned to_hide = mask & ~s_last_applied_mask;
  const unsigned to_show = s_last_applied_mask & ~mask;
  s_last_applied_mask = mask;

  if (auto* iface = cg_interface::get()) {
    if (to_show != 0) {
      calram_guide_mgr_wnd::apply_iface_promo_show(iface, static_cast<target>(to_show));
    }
    if (to_hide != 0) {
      calram_guide_mgr_wnd::apply_iface_promo_hide(iface, static_cast<target>(to_hide));
    }
    return;
  }

  if (auto* mgr = calram_guide_mgr_wnd::get_current()) {
    if (to_show != 0) {
      mgr->apply_promo_show(static_cast<target>(to_show));
    }
    if (to_hide != 0) {
      mgr->apply_promo_hide(static_cast<target>(to_hide));
    }
  }
}

} // namespace ext_client::plugins::hud_customizer
