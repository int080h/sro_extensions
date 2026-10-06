#pragma once

#include "sdk/process/cps_title.hpp"
#include "sdk/ui/cgwnd.hpp"

namespace ext_client::plugins::title {

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

  struct title_res_id {
    static constexpr int channel_message = 15;
    static constexpr int id_edit = 41;
    static constexpr int password_edit = 42;
    static constexpr int server_value = 43;
    static constexpr int server_button = 44;
    static constexpr int channel_value = 45;
    static constexpr int channel_button = 46;
    static constexpr int id_label = 101;
    static constexpr int password_label = 102;
    static constexpr int server_label = 103;
    static constexpr int channel_label = 104;
  };

  auto translated_rect(widget_rect rect, int x_adjust, int y_adjust) const -> widget_rect;
  auto fallback_rect_for_id(int res_id, widget_rect& out) const -> bool;
  auto apply_widget_rect(cps_title* title, int target_res_id, const widget_rect& rect, int base_x, int base_y) const -> bool;
  auto load_from_game(cps_title* title) -> bool;
  auto apply_eu_frame(cps_title* title, cgwnd* frame) const -> bool;
  auto restore_eu_frame(cps_title* title, cgwnd* frame) const -> bool;
};

auto show_title_child(cps_title* title, int res_id) -> void;
auto hide_title_child(cps_title* title, int res_id) -> void;
} // namespace ext_client::plugins::title
