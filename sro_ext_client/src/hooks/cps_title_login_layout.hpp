#pragma once

#include "utils/ui_res_catalog.hpp"

class cgwnd;
class cps_title;

namespace ext_client::hooks {

  class title_login_layout {
  public:
    struct widget_rect {
      int res_id = -1;
      int x = 0;
      int y = 0;
      int width = 0;
      int height = 0;
      bool valid = false;
    };

    widget_rect login_frame{};
    widget_rect id_label{};
    widget_rect id_edit{};
    widget_rect password_label{};
    widget_rect password_edit{};
    widget_rect server_label{};
    widget_rect server_value{};
    widget_rect server_button{};
    widget_rect channel_label{};
    widget_rect channel_value{};
    widget_rect channel_button{};
    widget_rect channel_name{};

    auto load_from_game(cps_title* title) -> bool;
    auto apply_eu_frame(cps_title* title, cgwnd* frame) const -> bool;

  private:
    auto load_named_or_id(const char* gdr_name, int fallback_res_id, widget_rect& out) -> bool;
  };

} // namespace ext_client::hooks
