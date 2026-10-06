#include "pch.hpp"
#include "plugins/title/title_layout.hpp"

#include "core/config.hpp"

namespace ext_client::plugins::title {

auto show_title_child(cps_title* title, int res_id) -> void {
  auto* wnd = title->find_child(res_id);
  if (wnd && wnd->is_live() && !wnd->is_visible()) {
    cgwnd::set_visible(wnd, true);
  }
}

auto hide_title_child(cps_title* title, int res_id) -> void {
  auto* wnd = title->find_child(res_id);
  if (wnd && wnd->is_live()) {
    cgwnd::set_visible(wnd, false);
  }
}

auto title_login_layout::translated_rect(widget_rect rect, int x_adjust, int y_adjust) const -> widget_rect {
  rect.x += x_adjust;
  rect.y += y_adjust;
  return rect;
}

auto title_login_layout::fallback_rect_for_id(int res_id, widget_rect& out) const -> bool {
  out = {res_id, 0, 0, 0, 0, true};
  switch (res_id) {
    case title_res_id::id_edit:
      out.x = 90;
      out.y = 29;
      out.width = 130;
      out.height = 20;
      return true;
    case title_res_id::password_edit:
      out.x = 90;
      out.y = 58;
      out.width = 186;
      out.height = 20;
      return true;
    case title_res_id::server_value:
      out.x = 90;
      out.y = 113;
      out.width = 186;
      out.height = 20;
      return true;
    case title_res_id::server_button:
      out.x = 284;
      out.y = 112;
      out.width = 48;
      out.height = 24;
      return true;
    case title_res_id::channel_value:
      out.x = 90;
      out.y = 85;
      out.width = 186;
      out.height = 20;
      return true;
    case title_res_id::channel_button:
      out.x = 284;
      out.y = 84;
      out.width = 48;
      out.height = 24;
      return true;
    case title_res_id::id_label:
      out.x = 14;
      out.y = 30;
      out.width = 70;
      out.height = 15;
      return true;
    case title_res_id::password_label:
      out.x = 14;
      out.y = 58;
      out.width = 70;
      out.height = 15;
      return true;
    case title_res_id::server_label:
      out.x = 14;
      out.y = 114;
      out.width = 70;
      out.height = 15;
      return true;
    case title_res_id::channel_label:
      out.x = 14;
      out.y = 86;
      out.width = 70;
      out.height = 15;
      return true;
    case title_res_id::channel_message:
      out.x = 0;
      out.y = 0;
      out.width = 339;
      out.height = 24;
      return true;
    default:
      out = {res_id, 0, 0, 0, 0, false};
      return false;
  }
}

auto title_login_layout::apply_widget_rect(cps_title* title, int target_res_id, const widget_rect& rect, int base_x, int base_y) const
  -> bool {
  auto* wnd = title->find_child(target_res_id);
  if (!wnd || !wnd->is_live()) {
    return false;
  }

  const int target_x = rect.valid ? base_x + rect.x : wnd->get_rect_x() + rect.x;
  const int target_y = rect.valid ? base_y + rect.y : wnd->get_rect_y() + rect.y;
  if (wnd->get_rect_x() != target_x || wnd->get_rect_y() != target_y) {
    cgwnd::set_position(wnd, target_x, target_y);
  }
  if (rect.valid && rect.width > 0 && rect.height > 0 && (wnd->get_rect_w() != rect.width || wnd->get_rect_h() != rect.height)) {
    cgwnd::set_size(wnd, rect.width, rect.height);
  }
  return true;
}

auto title_login_layout::load_from_game(cps_title* title) -> bool {
  if (!title) {
    return false;
  }

  bool ok = true;
  ok = fallback_rect_for_id(title_res_id::id_label, id_label) && ok;
  ok = fallback_rect_for_id(title_res_id::id_edit, id_edit) && ok;
  ok = fallback_rect_for_id(title_res_id::password_label, password_label) && ok;
  ok = fallback_rect_for_id(title_res_id::password_edit, password_edit) && ok;
  ok = fallback_rect_for_id(title_res_id::server_label, server_label) && ok;
  ok = fallback_rect_for_id(title_res_id::server_value, server_value) && ok;
  ok = fallback_rect_for_id(title_res_id::server_button, server_button) && ok;
  ok = fallback_rect_for_id(title_res_id::channel_label, channel_label) && ok;
  ok = fallback_rect_for_id(title_res_id::channel_value, channel_value) && ok;
  ok = fallback_rect_for_id(title_res_id::channel_button, channel_button) && ok;
  fallback_rect_for_id(title_res_id::channel_message, channel_name);
  return ok;
}

auto title_login_layout::apply_eu_frame(cps_title* title, cgwnd* frame) const -> bool {
  if (!title || !frame || !frame->is_live()) {
    return false;
  }
  const int base_x = frame->get_rect_x();
  const int base_y = frame->get_rect_y();
  const auto cfg_snap = ext_client::core::config::runtime();
  const auto& config = cfg_snap->title;
  bool moved = true;
  moved = apply_widget_rect(title, id_label.res_id, translated_rect(id_label, static_cast<int>(config.eu_login_id_label_adjust.x), static_cast<int>(config.eu_login_id_label_adjust.y)), base_x, base_y) && moved;
  moved = apply_widget_rect(title, id_edit.res_id, translated_rect(id_edit, static_cast<int>(config.eu_login_id_input_adjust.x), static_cast<int>(config.eu_login_id_input_adjust.y)), base_x, base_y) && moved;
  moved = apply_widget_rect(title, password_label.res_id, translated_rect(password_label, static_cast<int>(config.eu_login_pw_label_adjust.x), static_cast<int>(config.eu_login_pw_label_adjust.y)), base_x, base_y) && moved;
  moved = apply_widget_rect(title, password_edit.res_id, translated_rect(password_edit, static_cast<int>(config.eu_login_pw_input_adjust.x), static_cast<int>(config.eu_login_pw_input_adjust.y)), base_x, base_y) && moved;
  moved = apply_widget_rect(title, server_label.res_id, translated_rect(server_label, static_cast<int>(config.eu_login_server_label_adjust.x), static_cast<int>(config.eu_login_server_label_adjust.y)), base_x, base_y) && moved;
  auto eu_server_value = translated_rect(server_value, static_cast<int>(config.eu_login_server_value_adjust.x), static_cast<int>(config.eu_login_server_value_adjust.y));
  if (eu_server_value.width > 186) {
    eu_server_value.width = 186;
  }
  moved = apply_widget_rect(title, server_value.res_id, eu_server_value, base_x, base_y) && moved;
  moved = apply_widget_rect(title, server_button.res_id, translated_rect(server_button, static_cast<int>(config.eu_login_server_button_adjust.x), static_cast<int>(config.eu_login_server_button_adjust.y)), base_x, base_y) && moved;
  hide_title_child(title, channel_label.res_id);
  hide_title_child(title, channel_value.res_id);
  hide_title_child(title, channel_button.res_id);
  hide_title_child(title, channel_name.res_id);
  return moved;
}

auto title_login_layout::restore_eu_frame(cps_title* title, cgwnd* frame) const -> bool {
  if (!title || !frame || !frame->is_live()) {
    return false;
  }
  const int base_x = frame->get_rect_x();
  const int base_y = frame->get_rect_y();
  bool moved = true;
  moved = apply_widget_rect(title, id_label.res_id, id_label, base_x, base_y) && moved;
  moved = apply_widget_rect(title, id_edit.res_id, id_edit, base_x, base_y) && moved;
  moved = apply_widget_rect(title, password_label.res_id, password_label, base_x, base_y) && moved;
  moved = apply_widget_rect(title, password_edit.res_id, password_edit, base_x, base_y) && moved;
  moved = apply_widget_rect(title, server_label.res_id, server_label, base_x, base_y) && moved;
  moved = apply_widget_rect(title, server_value.res_id, server_value, base_x, base_y) && moved;
  moved = apply_widget_rect(title, server_button.res_id, server_button, base_x, base_y) && moved;
  show_title_child(title, channel_label.res_id);
  show_title_child(title, channel_value.res_id);
  show_title_child(title, channel_button.res_id);
  show_title_child(title, channel_name.res_id);

  return moved;
}
} // namespace ext_client::plugins::title
