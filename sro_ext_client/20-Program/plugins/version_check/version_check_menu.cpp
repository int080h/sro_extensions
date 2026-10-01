#include "pch.hpp"
#include "plugins/version_check/version_check_menu.hpp"

#include "core/core_config.hpp"
#include "render/menu_builder.hpp"

namespace ext_client::plugins::version_check {

auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void {
  ui.section("Loading Splash Slideshow Settings");

  auto& vc = ext_client::core::config::data().version_check;
  ui.checkbox("Enable Splash Slideshow", &vc.enabled);
  ui.checkbox("Banner Cycle Animation", &vc.banner_cycle);
  ui.checkbox("Banner GDI+ Splash Overlay", &vc.banner_overlay);
  ui.checkbox("Ensure Minimize Button", &vc.ensure_minimize_button);
  ui.slider_int("Cycle Duration (ms)", &vc.banner_cycle_interval_ms, 100, 5000);
}

} // namespace ext_client::plugins::version_check
