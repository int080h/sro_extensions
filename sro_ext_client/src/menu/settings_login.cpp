#include "menu/settings_login.hpp"

#include "config/client_config.hpp"
#include "hooks/title_hook.hpp"
#include "menu/menu_ui.hpp"
#include "sdk/cps_title.hpp"

#include <imgui.h>

namespace ext_client::menu::settings_login {
  namespace {

    constexpr ImGuiTableFlags k_two_column_flags = ImGuiTableFlags_SizingStretchSame;
    constexpr ImGuiTableFlags k_offset_table_flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV;

    auto reset_eu_login_layout(ext_client::config::settings::title_t& title) -> void {
      const ext_client::config::settings defaults{};
      title.eu_login_id_label_adjust = defaults.title.eu_login_id_label_adjust;
      title.eu_login_id_input_adjust = defaults.title.eu_login_id_input_adjust;
      title.eu_login_pw_label_adjust = defaults.title.eu_login_pw_label_adjust;
      title.eu_login_pw_input_adjust = defaults.title.eu_login_pw_input_adjust;
      title.eu_login_server_label_adjust = defaults.title.eu_login_server_label_adjust;
      title.eu_login_server_value_adjust = defaults.title.eu_login_server_value_adjust;
      title.eu_login_server_button_adjust = defaults.title.eu_login_server_button_adjust;
    }

    auto draw_eu_login_offsets(ext_client::config::settings::title_t& title) -> bool {
      bool changed = false;

      if (ImGui::BeginTable("eu_login_offsets", 2, k_offset_table_flags)) {
        ImGui::TableNextColumn();
        changed |= ext_client::menu::ui::offset_input2("ID label", title.eu_login_id_label_adjust);
        changed |= ext_client::menu::ui::offset_input2("PW label", title.eu_login_pw_label_adjust);
        changed |= ext_client::menu::ui::offset_input2("Server label", title.eu_login_server_label_adjust);

        ImGui::TableNextColumn();
        changed |= ext_client::menu::ui::offset_input2("ID input", title.eu_login_id_input_adjust);
        changed |= ext_client::menu::ui::offset_input2("PW input", title.eu_login_pw_input_adjust);
        changed |= ext_client::menu::ui::offset_input2("Server value", title.eu_login_server_value_adjust);
        changed |= ext_client::menu::ui::offset_input2("List button", title.eu_login_server_button_adjust);
        ImGui::EndTable();
      }

      if (ImGui::Button("Reset EU layout defaults##title_login_layout")) {
        reset_eu_login_layout(title);
        changed = true;
      }

      return changed;
    }

    auto draw_live_state() -> void {
      if (!ext_client::menu::ui::live_state_header("Live state")) {
        return;
      }

      ImGui::Text("captured %d/2 version labels", ext_client::hooks::title::captured_version_label_count());
      if (ImGui::Button("Apply labels")) {
        ext_client::hooks::title::apply_version_labels();
      }
      ImGui::SameLine();
      if (ImGui::Button("Apply layout")) {
        ext_client::hooks::title::apply_version_label_layout();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (auto* title = cps_title::current()) {
        ImGui::Text("instance %p  phase %d  captcha %s  ch %d",
                    title,
                    title->login_phase(),
                    title->captcha_active() ? "yes" : "no",
                    cps_title::channel_index());
        if (ImGui::Button("DoLogin")) {
          title->trigger_login();
        }
      } else {
        ImGui::TextDisabled("CPSTitle not active");
      }
    }

  } // namespace

  auto draw() -> void {
    ImGui::TextDisabled("CPSTitle hook overrides and EU login layout.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("login_card")) {
      auto& title = ext_client::config::data().title;
      bool changed = false;

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "title_main", &title.enabled, "Enabled", &title.log_events, "Log events");
      changed |= ext_client::menu::ui::checkbox_pair_table(
        "title_create", &title.skip_on_create, "Skip OnCreate", &title.auto_login_on_create, "Auto login");
      changed |= ext_client::menu::ui::checkbox_pair_table(
        "title_block", &title.block_login_packets, "Block login packets", &title.block_process_transition, "Block transition");

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "title_ui", &title.suppress_captcha, "Suppress captcha", &title.hide_channel_list_button, "Hide channel list");

      ext_client::menu::ui::two_column_table("title_toggle_grid", k_two_column_flags, [&]() {
        ImGui::TableNextColumn();
        changed |= ImGui::Checkbox("Override version text", &title.override_version_labels);
        changed |= ImGui::Checkbox("Replace login frame", &title.replace_login_frame);

        ImGui::TableNextColumn();
        changed |= ImGui::Checkbox("Override version color", &title.override_version_label_color);
        changed |= ImGui::Checkbox("Clip version labels", &title.version_labels_clip);
      });

      ext_client::menu::ui::set_full_width();
      changed |= ImGui::InputText("Login frame DDJ", title.login_frame_path, sizeof(title.login_frame_path));

      if (ext_client::menu::ui::section_header("EU login layout offsets")) {
        changed |= draw_eu_login_offsets(title);
      }

      ext_client::menu::ui::set_full_width();
      changed |= ImGui::InputText("Data version fmt", title.data_version_fmt, sizeof(title.data_version_fmt));
      ext_client::menu::ui::set_full_width();
      changed |= ImGui::InputText("Exe version fmt", title.exe_version_fmt, sizeof(title.exe_version_fmt));

      ext_client::menu::ui::two_column_table("title_numeric_grid", k_two_column_flags, [&]() {
        ImGui::TableNextColumn();
        ext_client::menu::ui::set_full_width();
        if (ImGui::InputInt("Logo Y offset", &title.logo_y_offset, 4, 20)) {
          if (title.logo_y_offset < 0) {
            title.logo_y_offset = 0;
          }
          changed = true;
        }

        ImGui::TableNextColumn();
        ext_client::menu::ui::set_full_width();
        if (ImGui::InputInt("Version clip width", &title.version_label_ellipsis_width, 4, 20)) {
          if (title.version_label_ellipsis_width < 16) {
            title.version_label_ellipsis_width = 16;
          }
          changed = true;
        }
      });

      if (title.override_version_label_color) {
        changed |= ext_client::menu::ui::sro_color_edit("Version label color", title.version_label_color);
      }

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }

      ImGui::Spacing();
      ext_client::menu::ui::apply_button("Apply now##title_config");
      ext_client::menu::ui::section_card_end();
    }

    ImGui::Spacing();
    draw_live_state();
  }

} // namespace ext_client::menu::settings_login
