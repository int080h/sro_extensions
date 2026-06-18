#pragma once

namespace ext_client::menu::settings_panels {

  enum class settings_page : int {
    iface = 0,
    version_check,
    login,
    graphics,
    welcome,
    advanced,
    app,
    count,
  };

  auto draw_sub_nav(settings_page& current) -> void;
  auto draw_page(settings_page current) -> void;

} // namespace ext_client::menu::settings_panels
