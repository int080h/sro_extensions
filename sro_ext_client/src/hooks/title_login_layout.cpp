#include "ext_client.hpp"

#include "hooks/title_login_layout.hpp"

#include "sdk/cgwnd.hpp"
#include "sdk/cif_manager.hpp"
#include "sdk/cps_title.hpp"

namespace ext_client::hooks::title {

  namespace {

    namespace title_res_id {
      constexpr int channel_message = 15;
      constexpr int id_edit = 41;
      constexpr int password_edit = 42;
      constexpr int server_value = 43;
      constexpr int server_button = 44;
      constexpr int channel_value = 45;
      constexpr int channel_button = 46;
      constexpr int id_label = 101;
      constexpr int password_label = 102;
      constexpr int server_label = 103;
      constexpr int channel_label = 104;
    } // namespace title_res_id

    auto title_ui_child(cps_title* title, int res_id) -> cgwnd* {
      return ext_client::cif_manager::find_title_child(title, res_id);
    }

    auto translated_rect(title_login_layout::widget_rect rect, int x_adjust, int y_adjust) -> title_login_layout::widget_rect {
      rect.x += x_adjust;
      rect.y += y_adjust;
      return rect;
    }

    auto fallback_rect_for_id(int res_id, title_login_layout::widget_rect& out) -> bool {
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

    auto apply_widget_rect(cps_title* title, int target_res_id, const title_login_layout::widget_rect& rect, int base_x, int base_y) -> bool {
      auto* wnd = title_ui_child(title, target_res_id);
      if (!cif_manager::is_live_widget(wnd)) {
        return false;
      }

      const int target_x = rect.valid ? base_x + rect.x : wnd->rect_x() + rect.x;
      const int target_y = rect.valid ? base_y + rect.y : wnd->rect_y() + rect.y;
      if (wnd->rect_x() != target_x || wnd->rect_y() != target_y) {
        cgwnd::set_position(wnd, target_x, target_y);
      }
      if (rect.valid && rect.width > 0 && rect.height > 0 && (wnd->rect_w() != rect.width || wnd->rect_h() != rect.height)) {
        cgwnd::set_size(wnd, rect.width, rect.height);
      }
      return true;
    }

    auto hide_title_child(cps_title* title, int res_id) -> void {
      auto* wnd = title_ui_child(title, res_id);
      if (!cif_manager::is_live_widget(wnd)) {
        return;
      }
      cgwnd::set_visible(wnd, false);
      wnd->m_visible = 0;
    }

  } // namespace

  auto title_login_layout::load_named_or_id(const char* gdr_name, int fallback_res_id, widget_rect& out) -> bool {
    const auto* entry = ext_client::utils::ui_res_catalog::find_by_name(ext_client::utils::ui_res_catalog::screen::pstitle, gdr_name);
    if (!entry) {
      entry = ext_client::utils::ui_res_catalog::find(ext_client::utils::ui_res_catalog::screen::pstitle, fallback_res_id);
    }
    if (!entry || entry->id < 0 || entry->rect_w <= 0 || entry->rect_h <= 0) {
      return fallback_rect_for_id(fallback_res_id, out);
    }

    out.res_id = fallback_res_id;
    out.x = entry->rect_x;
    out.y = entry->rect_y;
    out.width = entry->rect_w;
    out.height = entry->rect_h;
    out.valid = true;
    return true;
  }

  auto title_login_layout::load_from_game(cps_title* title) -> bool {
    if (!title) {
      return false;
    }

    ext_client::utils::ui_res_catalog::sync_from_game(title->res_ui_root(), ext_client::utils::ui_res_catalog::screen::pstitle);

    bool ok = true;
    ok = load_named_or_id("GDR_STATIC1", title_res_id::id_label, id_label) && ok;
    ok = load_named_or_id("GDR_EDIT_ID", title_res_id::id_edit, id_edit) && ok;
    ok = load_named_or_id("GDR_STATIC2", title_res_id::password_label, password_label) && ok;
    ok = load_named_or_id("GDR_EDIT_PASS", title_res_id::password_edit, password_edit) && ok;
    ok = load_named_or_id("GDR_STATIC3", title_res_id::server_label, server_label) && ok;
    ok = load_named_or_id("GDR_STA_SERVER", title_res_id::server_value, server_value) && ok;
    ok = load_named_or_id("GDR_BTN_SERVER", title_res_id::server_button, server_button) && ok;
    ok = load_named_or_id("GDR_STATIC4", title_res_id::channel_label, channel_label) && ok;
    ok = load_named_or_id("GDR_STA_CHANNEL", title_res_id::channel_value, channel_value) && ok;
    ok = load_named_or_id("GDR_BTN_CHANNEL", title_res_id::channel_button, channel_button) && ok;
    load_named_or_id("GDR_STA_CHANNELMESSAGE", title_res_id::channel_message, channel_name);
    (void)ok;
    return true;
  }

  auto title_login_layout::apply_eu_frame(cps_title* title, cgwnd* frame) const -> bool {
    if (!title || !cif_manager::is_live_widget(frame)) {
      return false;
    }

    const int base_x = frame->rect_x();
    const int base_y = frame->rect_y();
    const auto& config = ext_client::config::data().title;
    bool moved = true;

    moved = apply_widget_rect(title,
                              id_label.res_id,
                              translated_rect(id_label, static_cast<int>(config.eu_login_id_label_adjust.x), static_cast<int>(config.eu_login_id_label_adjust.y)),
                              base_x,
                              base_y) &&
            moved;
    moved = apply_widget_rect(title,
                              id_edit.res_id,
                              translated_rect(id_edit, static_cast<int>(config.eu_login_id_input_adjust.x), static_cast<int>(config.eu_login_id_input_adjust.y)),
                              base_x,
                              base_y) &&
            moved;
    moved = apply_widget_rect(title,
                              password_label.res_id,
                              translated_rect(password_label, static_cast<int>(config.eu_login_pw_label_adjust.x), static_cast<int>(config.eu_login_pw_label_adjust.y)),
                              base_x,
                              base_y) &&
            moved;
    moved = apply_widget_rect(title,
                              password_edit.res_id,
                              translated_rect(password_edit, static_cast<int>(config.eu_login_pw_input_adjust.x), static_cast<int>(config.eu_login_pw_input_adjust.y)),
                              base_x,
                              base_y) &&
            moved;

    moved = apply_widget_rect(title,
                              server_label.res_id,
                              translated_rect(server_label, static_cast<int>(config.eu_login_server_label_adjust.x), static_cast<int>(config.eu_login_server_label_adjust.y)),
                              base_x,
                              base_y) &&
            moved;
    auto eu_server_value = translated_rect(server_value, static_cast<int>(config.eu_login_server_value_adjust.x), static_cast<int>(config.eu_login_server_value_adjust.y));
    if (eu_server_value.width > 186) {
      eu_server_value.width = 186;
    }
    moved = apply_widget_rect(title,
                              server_value.res_id,
                              eu_server_value,
                              base_x,
                              base_y) &&
            moved;
    moved = apply_widget_rect(title,
                              server_button.res_id,
                              translated_rect(server_button, static_cast<int>(config.eu_login_server_button_adjust.x), static_cast<int>(config.eu_login_server_button_adjust.y)),
                              base_x,
                              base_y) &&
            moved;

    hide_title_child(title, channel_label.res_id);
    hide_title_child(title, channel_value.res_id);
    hide_title_child(title, channel_button.res_id);
    hide_title_child(title, channel_name.res_id);
    return moved;
  }

} // namespace ext_client::hooks::title
