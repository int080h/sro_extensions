#pragma once

#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "config/client_config.hpp"

namespace ext_client::hooks::interface_hide {

  using control = ext_client::config::settings::interface_hide_t;

  auto control_panel() -> control&;
  auto active_targets() -> calarm_guide_mgr_wnd::promo_target;
  auto any_hide_active() -> bool;

  auto install() -> bool;
  auto uninstall() -> void;
  auto is_installed() -> bool;
  auto apply_from_control() -> void;
  auto tick() -> void;

} // namespace ext_client::hooks::interface_hide
