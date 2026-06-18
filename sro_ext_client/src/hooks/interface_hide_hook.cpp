#include "ext_client.hpp"

#include "hooks/interface_hide_hook.hpp"

#include <Windows.h>
#include <cmath>
#include <cstdint>

#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cnif_sro_ingame_start.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"
#include "utils/tick_throttle.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::interface_hide {
  namespace {

    // ---------------------------------------------------------------------------
    // Promo/Alarm state
    // ---------------------------------------------------------------------------
    unsigned g_config_mask = 0;
    unsigned g_applied_mask = 0;
    bool g_promo_saw_ingame = false;
    std::uint32_t g_promo_enforce_counter = 0;
    constexpr std::uint32_t k_promo_enforce_every_n_ticks = 30;

    // ---------------------------------------------------------------------------
    // Survey state & hooks
    // ---------------------------------------------------------------------------
    make_hook<convention_type::thiscall_t, int, cnif_sro_ingame_start*, int, int> g_on_create;
    make_hook<convention_type::cdecl_t, char> g_try_show_panel;
    make_hook<convention_type::thiscall_t, int, cnif_sro_ingame_start*> g_arm_show_survey;

    hook_group g_hooks;

    std::uint32_t g_survey_enforce_counter = 0;
    bool g_survey_saw_ingame = false;
    bool g_survey_logged_resolve_miss = false;
    constexpr std::uint32_t k_survey_enforce_every_n_ticks = 30;

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------
    auto survey_hide_enabled() -> bool {
      return control_panel().hide_survey;
    }

    auto iface_if_ingame() -> cg_interface* {
      if (!cg_interface::is_ingame_hud_ready()) {
        calarm_guide_mgr_wnd::invalidate_cache();
        return nullptr;
      }
      return cg_interface::get();
    }

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

    auto apply_survey_hide_now() -> void {
      auto* iface = iface_if_ingame();
      if (!iface) {
        return;
      }

      const auto diag = cnif_sro_ingame_start::diagnose(iface);
      if (!diag.ready && !g_survey_logged_resolve_miss) {
        g_survey_logged_resolve_miss = true;
        log_msg("[interface_hide_hook] hide on; map 0x35 found=%d live=%d (hooks still block show)",
                diag.start_map.found ? 1 : 0,
                diag.start_map.live ? 1 : 0);
      }
      if (diag.ready) {
        g_survey_logged_resolve_miss = false;
      }

      cnif_sro_ingame_start::hide_survey_button(iface);
    }

    auto apply_survey_show_now() -> void {
      if (auto* iface = iface_if_ingame()) {
        cnif_sro_ingame_start::show_survey_button(iface);
      }
    }

    // ---------------------------------------------------------------------------
    // Detours
    // ---------------------------------------------------------------------------
    auto __fastcall on_create_detour(cnif_sro_ingame_start* self, void*, int a2, int a3) -> int {
      if (survey_hide_enabled()) {
        self->hide_start_panel();
        return 0;
      }

      return g_on_create.call_original(self, a2, a3);
    }

    auto __cdecl try_show_panel_detour() -> char {
      if (survey_hide_enabled()) {
        return 1;
      }

      return g_try_show_panel.call_original();
    }

    auto __fastcall arm_show_survey_detour(cnif_sro_ingame_start* self) -> int {
      if (survey_hide_enabled()) {
        return 0;
      }

      return g_arm_show_survey.call_original(self);
    }

  } // namespace

  auto control_panel() -> control& {
    return ext_client::config::data().interface_hide;
  }

  auto active_targets() -> calarm_guide_mgr_wnd::promo_target {
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

  auto any_hide_active() -> bool {
    const auto& cfg = control_panel();
    return cfg.hide_facebook || cfg.hide_magic_lamp || cfg.hide_daily_login || cfg.hide_web_item_alarm || cfg.hide_macro_guide ||
           cfg.hide_survey;
  }

  auto apply_promo_after_mgr_create(calarm_guide_mgr_wnd* mgr) -> void {
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

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets::cnif_sro_ingame_start::functions;
    if (!g_hooks.install(g_on_create, on_create, &on_create_detour, "interface_hide_hook", "on_create")) {
      return false;
    }
    if (!g_hooks.install(g_try_show_panel, try_show_panel, &try_show_panel_detour, "interface_hide_hook", "try_show_panel")) {
      g_hooks.uninstall();
      return false;
    }
    if (!g_hooks.install(g_arm_show_survey, arm_show_survey, &arm_show_survey_detour, "interface_hide_hook", "arm_show_survey")) {
      g_hooks.uninstall();
      return false;
    }

    log_msg("[interface_hide_hook] hooks installed");
    return true;
  }

  auto uninstall() -> void {
    if (!g_hooks.is_installed()) {
      return;
    }
    g_hooks.uninstall();
    log_msg("[interface_hide_hook] hooks removed");
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

  auto apply_from_control() -> void {
    g_config_mask = static_cast<unsigned>(active_targets());

    if (auto* iface = iface_if_ingame()) {
      apply_promo_mask(iface, g_config_mask);
    }

    if (survey_hide_enabled()) {
      apply_survey_hide_now();
    } else {
      apply_survey_show_now();
    }
  }

  auto tick() -> void {
    // 1. Tick calarm promo hide
    if (!cg_interface::is_ingame_hud_ready()) {
      calarm_guide_mgr_wnd::invalidate_cache();
      g_promo_saw_ingame = false;
      g_promo_enforce_counter = 0;
    } else {
      auto* iface = cg_interface::get();
      if (iface) {
        if (!g_promo_saw_ingame) {
          g_promo_saw_ingame = true;
          apply_promo_mask(iface, g_config_mask);
          g_promo_enforce_counter = 0;
        } else if (g_config_mask != 0) {
          if (ext_client::utils::every_n_ticks(g_promo_enforce_counter, k_promo_enforce_every_n_ticks)) {
            calarm_guide_mgr_wnd::apply_iface_promo_hide(iface, static_cast<calarm_guide_mgr_wnd::promo_target>(g_config_mask));
            g_applied_mask = g_config_mask;
          }
        }
      }
    }

    // 2. Tick survey hide
    if (!survey_hide_enabled()) {
      g_survey_saw_ingame = false;
      g_survey_enforce_counter = 0;
      g_survey_logged_resolve_miss = false;
      return;
    }

    if (!iface_if_ingame()) {
      g_survey_saw_ingame = false;
      g_survey_enforce_counter = 0;
      return;
    }

    if (!g_survey_saw_ingame) {
      g_survey_saw_ingame = true;
      g_survey_enforce_counter = 0;
      apply_survey_hide_now();
      return;
    }

    if (ext_client::utils::every_n_ticks(g_survey_enforce_counter, k_survey_enforce_every_n_ticks)) {
      apply_survey_hide_now();
    }
  }

} // namespace ext_client::hooks::interface_hide
