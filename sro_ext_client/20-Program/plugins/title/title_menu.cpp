#include "pch.hpp"
#include "plugins/title/title_runtime.hpp"

#include "core/core_config.hpp"
#include "render/menu_builder.hpp"

#include <imgui.h>
#include <cstdint>

namespace ext_client::plugins::title {

  auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void {
    ui.section("Custom Login Visuals Settings");

    auto& title = ext_client::core::config::data().title;

    ui.checkbox("Enable Custom Login Visuals", &title.enabled);
    ui.checkbox("Hide Channel List Button", &title.hide_channel_list_button);
    ui.checkbox("Replace Login Background Frame", &title.replace_login_frame);
    if (title.replace_login_frame) {
      ui.set_next_item_width(ImGui::GetContentRegionAvail().x - 20.0f);
      ui.input_text("Login DDJ Image Path", title.login_frame_path, sizeof(title.login_frame_path));
    }

    ui.slider_int("Logo Y Offset", &title.logo_y_offset, -200, 200);

    ui.spacing();
    if (ui.collapsing_header("Version Label Customization")) {
      ui.checkbox("Override System Version Labels", &title.override_version_labels);
      if (title.override_version_labels) {
        ui.set_next_item_width(ImGui::GetContentRegionAvail().x - 40.0f);
        ui.input_text("Data Version Format String", title.data_version_fmt, sizeof(title.data_version_fmt));
        ui.set_next_item_width(ImGui::GetContentRegionAvail().x - 40.0f);
        ui.input_text("Exe Version Format String", title.exe_version_fmt, sizeof(title.exe_version_fmt));
      }
      ui.checkbox("Override Version Label Color", &title.override_version_label_color);
      if (title.override_version_label_color) {
        float col[4] = {
          ((title.version_label_color & 0x00FF0000) >> 16) / 255.f,
          ((title.version_label_color & 0x0000FF00) >> 8) / 255.f,
          ((title.version_label_color & 0x000000FF) >> 0) / 255.f,
          ((title.version_label_color & 0xFF000000) >> 24) / 255.f};
        if (ImGui::ColorEdit4("Version Label Color", col)) {
          title.version_label_color =
            (static_cast<std::uint32_t>(col[3] * 255.f + 0.5f) << 24) |
            (static_cast<std::uint32_t>(col[0] * 255.f + 0.5f) << 16) |
            (static_cast<std::uint32_t>(col[1] * 255.f + 0.5f) << 8) |
            (static_cast<std::uint32_t>(col[2] * 255.f + 0.5f) << 0);
          ui.note_dirty();
        }
      }
      ui.checkbox("Clip Version Labels", &title.version_labels_clip);
      if (title.version_labels_clip) {
        ui.slider_int("Ellipsis Clip Width", &title.version_label_ellipsis_width, 10, 300);
      }
    }

    ui.spacing();
    if (ui.collapsing_header("EU Frame Layout Position Adjustments")) {
      float val[2];

      val[0] = title.eu_login_id_label_adjust.x;
      val[1] = title.eu_login_id_label_adjust.y;
      if (ui.drag_float2("ID Label Adjust", val)) {
        title.eu_login_id_label_adjust.x = val[0];
        title.eu_login_id_label_adjust.y = val[1];
      }

      val[0] = title.eu_login_id_input_adjust.x;
      val[1] = title.eu_login_id_input_adjust.y;
      if (ui.drag_float2("ID Input Adjust", val)) {
        title.eu_login_id_input_adjust.x = val[0];
        title.eu_login_id_input_adjust.y = val[1];
      }

      val[0] = title.eu_login_pw_label_adjust.x;
      val[1] = title.eu_login_pw_label_adjust.y;
      if (ui.drag_float2("PW Label Adjust", val)) {
        title.eu_login_pw_label_adjust.x = val[0];
        title.eu_login_pw_label_adjust.y = val[1];
      }

      val[0] = title.eu_login_pw_input_adjust.x;
      val[1] = title.eu_login_pw_input_adjust.y;
      if (ui.drag_float2("PW Input Adjust", val)) {
        title.eu_login_pw_input_adjust.x = val[0];
        title.eu_login_pw_input_adjust.y = val[1];
      }

      val[0] = title.eu_login_server_label_adjust.x;
      val[1] = title.eu_login_server_label_adjust.y;
      if (ui.drag_float2("ServerLabel Adjust", val)) {
        title.eu_login_server_label_adjust.x = val[0];
        title.eu_login_server_label_adjust.y = val[1];
      }

      val[0] = title.eu_login_server_value_adjust.x;
      val[1] = title.eu_login_server_value_adjust.y;
      if (ui.drag_float2("Server Value Adjust", val)) {
        title.eu_login_server_value_adjust.x = val[0];
        title.eu_login_server_value_adjust.y = val[1];
      }

      val[0] = title.eu_login_server_button_adjust.x;
      val[1] = title.eu_login_server_button_adjust.y;
      if (ui.drag_float2("Server Button Adjust", val)) {
        title.eu_login_server_button_adjust.x = val[0];
        title.eu_login_server_button_adjust.y = val[1];
      }
    }

    if (ui.any_changed()) {
      apply_from_control();
    }
  }

} // namespace ext_client::plugins::title
