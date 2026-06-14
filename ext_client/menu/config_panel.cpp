#include "menu/config_panel.hpp"

#include "config/client_config.hpp"
#include "hooks/interface_hide_hook.hpp"
#include "hooks/sight_range_hook.hpp"
#include "hooks/title_hook.hpp"
#include "menu/menu_ui.hpp"
#include "sdk/calarm_guide_mgr_wnd.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cnif_sro_ingame_start.hpp"
#include "sdk/sworld.hpp"
#include "utils/offsets.hpp"

#include <imgui.h>

namespace ext_client::menu::config_panel {
  namespace {

    constexpr ImGuiTableFlags k_two_column_flags = ImGuiTableFlags_SizingStretchSame;
    constexpr ImGuiTableFlags k_offset_table_flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV;

    auto offset_input(const char* label, float* x, float* y) -> bool {
      int values[2] = {static_cast<int>(*x), static_cast<int>(*y)};
      ImGui::SetNextItemWidth(-1.0f);
      if (!ImGui::InputInt2(label, values)) {
        return false;
      }
      *x = static_cast<float>(values[0]);
      *y = static_cast<float>(values[1]);
      return true;
    }

    auto set_full_width() -> void {
      ImGui::SetNextItemWidth(-1.0f);
    }

    auto mark_runtime_dirty() -> void {
      ext_client::menu::ui::setting_changed();
    }

    auto title_changed(bool changed) -> void {
      if (!changed) {
        return;
      }
      mark_runtime_dirty();
      ext_client::hooks::title::apply_from_control();
    }

    auto draw_section_spacing() -> void {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
    }

    auto draw_checkbox_pair(const char* left_label, bool* left, const char* right_label, bool* right) -> void {
      ImGui::TableNextColumn();
      ext_client::menu::ui::checkbox_setting(left_label, left);
      ImGui::TableNextColumn();
      ext_client::menu::ui::checkbox_setting(right_label, right);
    }

    auto draw_interface_section() -> void {
      if (!ImGui::CollapsingHeader("Interface hiding", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      auto& iface = ext_client::config::data().interface_hide;
      if (ImGui::BeginTable("interface_hide_grid", 2, k_two_column_flags)) {
        draw_checkbox_pair("Facebook alarm", &iface.hide_facebook, "Magic lamp alarm", &iface.hide_magic_lamp);
        draw_checkbox_pair("Daily login alarm", &iface.hide_daily_login, "Web item alarm", &iface.hide_web_item_alarm);
        draw_checkbox_pair("Macro guide alarm", &iface.hide_macro_guide, "Survey button", &iface.hide_survey);
        ImGui::TableNextColumn();
        ext_client::menu::ui::checkbox_setting("Apply on startup", &iface.apply_on_startup);
        ImGui::EndTable();
      }

      ImGui::Spacing();
      if (ImGui::Button("Apply now##iface")) {
        ext_client::menu::ui::setting_changed();
      }
      ImGui::SameLine();
      if (cg_interface::is_ingame_hud_ready()) {
        const auto diag = cnif_sro_ingame_start::diagnose(cg_interface::get());
        ImGui::TextDisabled("survey map@0x35 start=%p btn(uid3)=%p | info@0x34=%p | show=%u",
                            diag.live.start_panel,
                            diag.live.survey_button,
                            diag.live.info_panel,
                            diag.live.start_panel != nullptr ? (diag.live.start_panel->show_survey() ? 1u : 0u) : 0u);
        if (!diag.start_map.map_readable) {
          ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f),
                             "ingame map unreadable @ 0x%08X",
                             ext_client::offsets::ingame_ui_map::globals::address);
        } else if (iface.hide_survey && !diag.ready) {
          ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "hide on but widgets not resolved (hooks still block show)");
        }
        if (diag.start_map.found && !cnif_sro_ingame_start::is_instance(diag.start_map.raw)) {
          ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "map 0x35 vft mismatch (got 0x%08X)", diag.start_map.vftable);
        }
      } else {
        ImGui::TextDisabled("not in-game");
      }

      const auto& ctrl = ext_client::hooks::interface_hide::control_panel();
      ImGui::TextDisabled("active: facebook=%s magic_lamp=%s daily_login=%s web_item_alarm=%s macro_guide=%s survey=%s",
                          ctrl.hide_facebook ? "on" : "off",
                          ctrl.hide_magic_lamp ? "on" : "off",
                          ctrl.hide_daily_login ? "on" : "off",
                          ctrl.hide_web_item_alarm ? "on" : "off",
                          ctrl.hide_macro_guide ? "on" : "off",
                          ctrl.hide_survey ? "on" : "off");
    }

    auto reset_eu_login_layout(ext_client::config::settings::title_t& title) -> void {
      title.eu_login_id_label_adjust = vector2f{64.f, 0.f};
      title.eu_login_id_input_adjust = vector2f{22.f, 0.f};
      title.eu_login_pw_label_adjust = vector2f{64.f, 0.f};
      title.eu_login_pw_input_adjust = vector2f{22.f, 0.f};
      title.eu_login_server_label_adjust = vector2f{64.f, -8.f};
      title.eu_login_server_value_adjust = vector2f{18.f, -8.f};
      title.eu_login_server_button_adjust = vector2f{-20.f, -8.f};
    }

    auto draw_eu_login_offsets(ext_client::config::settings::title_t& title) -> bool {
      bool changed = false;

      if (ImGui::BeginTable("eu_login_offsets", 2, k_offset_table_flags)) {
        ImGui::TableNextColumn();
        changed |= offset_input("ID label", &title.eu_login_id_label_adjust.x, &title.eu_login_id_label_adjust.y);
        changed |= offset_input("PW label", &title.eu_login_pw_label_adjust.x, &title.eu_login_pw_label_adjust.y);
        changed |= offset_input("Server label", &title.eu_login_server_label_adjust.x, &title.eu_login_server_label_adjust.y);

        ImGui::TableNextColumn();
        changed |= offset_input("ID input", &title.eu_login_id_input_adjust.x, &title.eu_login_id_input_adjust.y);
        changed |= offset_input("PW input", &title.eu_login_pw_input_adjust.x, &title.eu_login_pw_input_adjust.y);
        changed |= offset_input("Server value", &title.eu_login_server_value_adjust.x, &title.eu_login_server_value_adjust.y);
        changed |= offset_input("List button", &title.eu_login_server_button_adjust.x, &title.eu_login_server_button_adjust.y);
        ImGui::EndTable();
      }

      if (ImGui::Button("Reset EU layout defaults##title_login_layout")) {
        reset_eu_login_layout(title);
        changed = true;
      }

      return changed;
    }

    auto draw_title_section() -> void {
      if (!ImGui::CollapsingHeader("Title overrides", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      auto& title = ext_client::config::data().title;
      bool changed = false;

      if (ImGui::BeginTable("title_toggle_grid", 2, k_two_column_flags)) {
        ImGui::TableNextColumn();
        changed |= ImGui::Checkbox("Override version text", &title.override_version_labels);
        changed |= ImGui::Checkbox("Replace login frame", &title.replace_login_frame);

        ImGui::TableNextColumn();
        changed |= ImGui::Checkbox("Override version color", &title.override_version_label_color);
        changed |= ImGui::Checkbox("Clip version labels", &title.version_labels_clip);
        ImGui::EndTable();
      }

      set_full_width();
      changed |= ImGui::InputText("Login frame DDJ", title.login_frame_path, sizeof(title.login_frame_path));

      if (ImGui::CollapsingHeader("EU login layout offsets", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= draw_eu_login_offsets(title);
      }

      set_full_width();
      changed |= ImGui::InputText("Data version fmt", title.data_version_fmt, sizeof(title.data_version_fmt));
      set_full_width();
      changed |= ImGui::InputText("Exe version fmt", title.exe_version_fmt, sizeof(title.exe_version_fmt));

      if (ImGui::BeginTable("title_numeric_grid", 2, k_two_column_flags)) {
        ImGui::TableNextColumn();
        set_full_width();
        if (ImGui::InputInt("Logo Y offset", &title.logo_y_offset, 4, 20)) {
          if (title.logo_y_offset < 0) {
            title.logo_y_offset = 0;
          }
          changed = true;
        }

        ImGui::TableNextColumn();
        set_full_width();
        if (ImGui::InputInt("Version clip width", &title.version_label_ellipsis_width, 4, 20)) {
          if (title.version_label_ellipsis_width < 16) {
            title.version_label_ellipsis_width = 16;
          }
          changed = true;
        }
        ImGui::EndTable();
      }

      ImGui::Spacing();
      if (ImGui::Button("Apply now##title_config")) {
        changed = true;
      }

      title_changed(changed);
    }

    auto draw_widgets_section() -> void {
      if (!ImGui::CollapsingHeader("Widget inspector defaults", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      auto& widgets = ext_client::config::data().widgets;
      if (ImGui::BeginTable("widget_defaults_grid", 2, k_two_column_flags)) {
        draw_checkbox_pair("Static texts only", &widgets.static_only, "Auto refresh list", &widgets.auto_refresh);
        ImGui::TableNextColumn();
        ext_client::menu::ui::checkbox_setting("Show alarm slot debug", &widgets.show_alarm_debug);
        ImGui::EndTable();
      }
      ImGui::SetNextItemWidth(220.0f);
      if (ImGui::SliderInt("Walk depth", &widgets.max_depth, 1, 48)) {
        ext_client::menu::ui::setting_changed(false);
      }
    }

    auto draw_graphics_section() -> void {
      if (!ImGui::CollapsingHeader("Graphics Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
      }

      auto& graphic = ext_client::config::data().graphic;
      bool changed = false;

      if (ImGui::Checkbox("Enable sight range override", &graphic.custom_sight_range)) {
        changed = true;
      }

      if (graphic.custom_sight_range) {
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::SliderInt("Override Percentage", &graphic.sight_range_percent, 50, 400, "%d%%")) {
          changed = true;
        }

        float factor = graphic.sight_range_percent / 100.0f;
        float range = 5500.0f * factor;
        std::int32_t cell = static_cast<std::int32_t>(2048.0f * factor);
        float fade = range * 0.8f;

        ImGui::TextDisabled("apply range %.1f | cell %d | fade %.1f", range, cell, fade);
      } else {
        ImGui::TextDisabled("default: dropdown setting 5 auto-upgrades to level 6");
      }

      ImGui::Spacing();
      ImGui::Text("Active values");
      auto* world = sworld::instance();
      if (world && sworld::is_instance()) {
        float active_range = world->sight_range();
        std::int32_t active_cell = world->cell_limit();
        float active_fade = ext_client::offsets::global_at<float>(ext_client::offsets::sworld::globals::details_fade_distance);

        int current_opt = world->option(8);

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                           "option %d (index %d) | range %.1f | cell %d | fade %.1f",
                           current_opt + 1,
                           current_opt,
                           active_range,
                           active_cell,
                           active_fade);
      } else {
        ImGui::TextDisabled("world not loaded");
      }

      if (changed) {
        ext_client::menu::ui::setting_changed(false);
        ext_client::hooks::sight_range::apply_settings();
      }
    }

    auto draw_general_section() -> void {
      if (!ImGui::CollapsingHeader("General")) {
        return;
      }

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
    }

  } // namespace

  auto draw() -> void {
    ImGui::BeginChild("config_scroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    draw_interface_section();
    draw_section_spacing();
    draw_title_section();
    draw_section_spacing();
    draw_graphics_section();
    draw_section_spacing();
    draw_widgets_section();
    draw_section_spacing();
    draw_general_section();
    ImGui::EndChild();
  }

} // namespace ext_client::menu::config_panel
