#pragma once

#include "ext_client.hpp"
#include "sdk/calarm_guide_mgr_wnd.hpp"

namespace ext_client::hooks {

  class cnif_sro_ingame_start_hook {
  public:
    using control = ext_client::config::settings::interface_hide_t;

    static auto control_panel() -> control&;
    static auto active_targets() -> calarm_guide_mgr_wnd::promo_target;
    static auto any_hide_active() -> bool;

    static auto install() -> bool;
    static auto uninstall() -> void;
    static auto is_installed() -> bool;
    static auto apply_from_control() -> void;
    static auto apply_promo_after_mgr_create(calarm_guide_mgr_wnd* mgr) -> void;
    static auto tick() -> void;
  };

} // namespace ext_client::hooks
