#pragma once

#include <vector>

namespace ext_client::menu::settings_panels {

  struct panel_info {
    const char* nav_label;
    void (*draw)();
  };

  auto panels() -> const std::vector<panel_info>&;
  auto draw_sub_nav(int& current) -> void;
  auto draw_page(int current) -> void;

} // namespace ext_client::menu::settings_panels
