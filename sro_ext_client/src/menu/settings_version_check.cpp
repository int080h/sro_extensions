#include "ext_client.hpp"

#include "menu/settings_version_check.hpp"

#include "menu/menu_ui.hpp"
#include "sdk/cps_version_check.hpp"

#include <imgui.h>

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
