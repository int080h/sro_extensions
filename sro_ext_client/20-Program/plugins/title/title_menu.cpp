#include "pch.hpp"
#include "plugins/title/title_runtime.hpp"

#include "core/config.hpp"
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
        ui.color_edit4_argb("Version Label Color", title.version_label_color);
      }
      ui.checkbox("Clip Version Labels", &title.version_labels_clip);
      if (title.version_labels_clip) {
        ui.slider_int("Ellipsis Clip Width", &title.version_label_ellipsis_width, 10, 300);
      }
    }

    ui.spacing();
    if (ui.collapsing_header("EU Frame Layout Position Adjustments")) {
      const struct {
        const char* label;
        vector2f* val;
      } adjustments[] = {
        {"ID Label Adjust", &title.eu_login_id_label_adjust},
        {"ID Input Adjust", &title.eu_login_id_input_adjust},
        {"PW Label Adjust", &title.eu_login_pw_label_adjust},
        {"PW Input Adjust", &title.eu_login_pw_input_adjust},
        {"ServerLabel Adjust", &title.eu_login_server_label_adjust},
        {"Server Value Adjust", &title.eu_login_server_value_adjust},
        {"Server Button Adjust", &title.eu_login_server_button_adjust},
      };

      for (const auto& item : adjustments) {
        ui.drag_float2(item.label, *item.val);
      }
    }

    if (ui.any_changed()) {
      apply_from_control();
    }
  }
} // namespace ext_client::plugins::title
