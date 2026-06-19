#include "ext_client.hpp"
#include "menu/settings_panels.hpp"
#include "menu/menu.hpp"

#include "sdk/cps_version_check.hpp"
#include "sdk/cps_title.hpp"
#include "hooks/cps_title_hook.hpp"
#include "utils/offsets.hpp"

#include <imgui.h>

// ===========================================================================
// settings_version_check
// ===========================================================================

namespace ext_client::menu::settings_version_check {
  namespace {

    auto draw_live_state() -> void {
      if (!ext_client::menu::ui::live_state_header("Live state")) {
        return;
      }

      if (auto* vc = cps_version_check::current()) {
        ImGui::Text("instance %p  net %d  load %s  active %s",
                    vc,
                    vc->net_state(),
                    vc->data_load_started() ? "yes" : "no",
                    cps_version_check::is_active() ? "yes" : "no");
      } else {
        ImGui::TextDisabled("instance (none)");
      }
      ImGui::Text("error %d  tag %d", cps_version_check::version_error_code(), cps_version_check::version_error_tag());
    }

    auto draw_banner_settings(ext_client::config::settings::version_check_t& cfg) -> bool {
      bool changed = false;

      if (ext_client::menu::ui::section_header("Loading banner")) {
        changed |= ext_client::menu::ui::checkbox_setting("Custom size", &cfg.banner_custom_size);
        changed |= ext_client::menu::ui::checkbox_setting("Center on screen", &cfg.banner_center);
        changed |= ext_client::menu::ui::checkbox_setting("Cycle images", &cfg.banner_cycle);
        changed |= ext_client::menu::ui::checkbox_setting("Loading overlay", &cfg.banner_overlay);

        if (cfg.banner_custom_size) {
          ext_client::menu::ui::set_full_width();
          if (ImGui::InputInt("Banner width", &cfg.banner_width)) {
            changed = true;
          }
          ext_client::menu::ui::set_full_width();
          if (ImGui::InputInt("Banner height", &cfg.banner_height)) {
            changed = true;
          }
        }

        if (!cfg.banner_center) {
          ext_client::menu::ui::set_full_width();
          if (ImGui::InputInt("Banner X", &cfg.banner_x)) {
            changed = true;
          }
          ext_client::menu::ui::set_full_width();
          if (ImGui::InputInt("Banner Y", &cfg.banner_y)) {
            changed = true;
          }
        }

        if (cfg.banner_cycle) {
          ext_client::menu::ui::set_full_width();
          if (ImGui::InputInt("Cycle interval (ms)", &cfg.banner_cycle_interval_ms)) {
            changed = true;
          }
          ext_client::menu::ui::set_full_width();
          if (ImGui::InputInt("Image count", &cfg.banner_count)) {
            changed = true;
          }
        }

        ext_client::menu::ui::set_full_width();
        if (ImGui::InputText("Path format", cfg.banner_path_fmt, sizeof(cfg.banner_path_fmt))) {
          changed = true;
        }
      }

      return changed;
    }

  } // namespace

  auto draw() -> void {
    ImGui::TextDisabled("CPSVersionCheck hook and loading splash behavior.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("vc_card")) {
      auto& cfg = ext_client::config::data().version_check;
      bool changed = false;

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "vc_main", &cfg.enabled, "Enabled", &cfg.log_events, "Log events");
      changed |= ext_client::menu::ui::checkbox_pair_table(
        "vc_gateway", &cfg.bypass_gateway_connect, "Bypass gateway", &cfg.skip_textdata_load, "Skip textdata");
      changed |= ext_client::menu::ui::checkbox_pair_table(
        "vc_transition", &cfg.block_process_transition, "Block transition", &cfg.skip_loading_splash, "Skip splash");

      if (ImGui::Checkbox("Force version result", &cfg.force_version_result)) {
        changed = true;
      }
      ImGui::SetNextItemWidth(90.0f);
      if (ImGui::InputInt("Result##vc_cfg", &cfg.forced_version_result)) {
        changed = true;
      }

      changed |= draw_banner_settings(cfg);

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }

      ImGui::Spacing();
      ext_client::menu::ui::apply_button("Apply now##vc");
      ext_client::menu::ui::section_card_end();
    }

    ImGui::Spacing();
    draw_live_state();
  }

} // namespace ext_client::menu::settings_version_check

// ===========================================================================
// settings_login
// ===========================================================================

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

      ImGui::Text("captured %d/2 version labels", ext_client::hooks::cps_title_hook::captured_version_label_count());
      if (ImGui::Button("Apply labels")) {
        ext_client::hooks::cps_title_hook::apply_version_labels();
      }
      ImGui::SameLine();
      if (ImGui::Button("Apply layout")) {
        ext_client::hooks::cps_title_hook::apply_version_label_layout();
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

// ===========================================================================
// settings_graphics
// ===========================================================================

namespace ext_client::menu::settings_graphics {
  namespace {

    constexpr ImGuiTableFlags k_two_column_flags = ImGuiTableFlags_SizingStretchSame;

  } // namespace

  auto draw() -> void {
    ImGui::TextDisabled("D3D9 presentation flags.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("gfx_d3d_card")) {
      auto& graphic = ext_client::config::data().graphic;
      bool changed = false;

      ext_client::menu::ui::two_column_table("d3d_flags", k_two_column_flags, [&]() {
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("Force hardware VP", &graphic.d3d_force_hardware_vp)) {
          changed = true;
        }
        if (ImGui::Checkbox("Triple buffering", &graphic.d3d_triple_buffering)) {
          changed = true;
        }

        ImGui::TableNextColumn();
        if (ImGui::Checkbox("Force pure device", &graphic.d3d_force_pure_device)) {
          changed = true;
        }
        if (ImGui::Checkbox("Discard depth stencil", &graphic.d3d_discard_depth_stencil)) {
          changed = true;
        }
      });

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }
      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_graphics

// ===========================================================================
// settings_welcome
// ===========================================================================

namespace ext_client::menu::settings_welcome {

  auto draw() -> void {
    ImGui::TextDisabled("Custom welcome message shown on login.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("welcome_card")) {
      auto& welcome = ext_client::config::data().welcome_msg;

      ext_client::menu::ui::checkbox_setting("Enabled", &welcome.enabled);
      ext_client::menu::ui::text_input_setting("Message", welcome.text, sizeof(welcome.text));

      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_welcome

// ===========================================================================
// settings_advanced
// ===========================================================================

namespace ext_client::menu::settings_advanced {
  namespace {

    constexpr ImGuiTableFlags k_two_column_flags = ImGuiTableFlags_SizingStretchSame;

    auto draw_checkbox_pair(const char* left_label, bool* left, const char* right_label, bool* right) -> void {
      ImGui::TableNextColumn();
      ext_client::menu::ui::checkbox_setting(left_label, left);
      ImGui::TableNextColumn();
      ext_client::menu::ui::checkbox_setting(right_label, right);
    }

    auto draw_packet_defaults() -> void {
      if (!ext_client::menu::ui::section_header("Packet hook defaults")) {
        return;
      }

      auto& packet = ext_client::config::data().net;
      bool changed = false;

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "pkt_main", &packet.enabled, "Hook enabled", &packet.log_events, "Log events");

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "pkt_out", &packet.edit_outgoing, "Edit outgoing", &packet.edit_outgoing_apply_all, "Apply all outgoing");

      changed |= ext_client::menu::ui::checkbox_pair_table(
        "pkt_in", &packet.edit_incoming, "Edit incoming", &packet.edit_incoming_apply_all, "Apply all incoming");

      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::InputScalar("Outgoing opcode", ImGuiDataType_U16, &packet.edit_outgoing_opcode, nullptr, nullptr, "%04X")) {
        changed = true;
      }
      ImGui::SetNextItemWidth(100.0f);
      if (ImGui::InputScalar("Incoming opcode", ImGuiDataType_U16, &packet.edit_incoming_opcode, nullptr, nullptr, "%04X")) {
        changed = true;
      }

      if (changed) {
        ext_client::menu::ui::setting_changed();
      }
    }

    auto draw_widget_defaults() -> void {
      if (!ext_client::menu::ui::section_header("Widget inspector defaults")) {
        return;
      }

      auto& widgets = ext_client::config::data().widgets;

      ext_client::menu::ui::two_column_table("widget_defaults_grid", k_two_column_flags, [&]() {
        draw_checkbox_pair("Static texts only", &widgets.static_only, "Auto refresh list", &widgets.auto_refresh);
        ImGui::TableNextColumn();
        ext_client::menu::ui::checkbox_setting("Show alarm slot debug", &widgets.show_alarm_debug);
      });

      ImGui::TextDisabled("Widget walk follows game child lists + res maps (VC2005); no depth cap.");
    }

  } // namespace

  auto draw() -> void {
    ImGui::TextDisabled("Packet hook INI defaults and widget inspector preferences.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("advanced_card")) {
      draw_packet_defaults();
      ImGui::Spacing();
      draw_widget_defaults();
      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_advanced

// ===========================================================================
// settings_app
// ===========================================================================

namespace ext_client::menu::settings_app {

  auto draw() -> void {
    ImGui::TextDisabled("Configuration file and extension lifecycle.");
    ImGui::Spacing();

    if (ext_client::menu::ui::section_card_begin("app_card")) {
      auto& general = ext_client::config::data().general;
      if (ImGui::Checkbox("Save on change", &general.save_on_change)) {
        ext_client::config::save();
      }

      ImGui::Spacing();
      ImGui::TextDisabled("config: %s", ext_client::config::path());
      if (ImGui::Button("Save##config")) {
        ext_client::config::save();
      }
      ImGui::SameLine();
      if (ImGui::Button("Reload##config")) {
        ext_client::config::load();
        ext_client::config::sync_to_runtime();
      }

      ImGui::Spacing();
      if (ImGui::Button("Unload extension (F7)")) {
        ext_client::core::get().request_unload();
      }

      ext_client::menu::ui::section_card_end();
    }
  }

} // namespace ext_client::menu::settings_app

// ===========================================================================
// settings_panels registry + dispatch
// ===========================================================================

namespace ext_client::menu::settings_panels {

  auto panels() -> const std::vector<panel_info>& {
    static const std::vector<panel_info> k_panels = {
      {"Version Check", settings_version_check::draw},
      {"Login",         settings_login::draw},
      {"Graphics",      settings_graphics::draw},
      {"Welcome",       settings_welcome::draw},
      {"Advanced",      settings_advanced::draw},
      {"App",           settings_app::draw},
    };
    return k_panels;
  }

  auto draw_sub_nav(int& current) -> void {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
    const auto& list = panels();
    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
      const bool selected = current == i;
      if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_TabSelected));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabSelected));
      }
      if (ImGui::Button(list[static_cast<std::size_t>(i)].nav_label, ImVec2(-1.0f, 0.0f))) {
        current = i;
      }
      if (selected) {
        ImGui::PopStyleColor(2);
      }
    }
    ImGui::PopStyleVar();
  }

  auto draw_page(int current) -> void {
    const auto& list = panels();
    if (current >= 0 && current < static_cast<int>(list.size())) {
      list[static_cast<std::size_t>(current)].draw();
    }
  }

} // namespace ext_client::menu::settings_panels
