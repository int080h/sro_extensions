#include "pch.hpp"
#include "plugins/hud_customizer/hud_customizer_plugin.hpp"

#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "render/menu_builder.hpp"
#include "plugins/hud_customizer/promo_hide.hpp"
#include "sdk/process/cps_outer_interface.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "sdk/ui/cnif_sro_ingame_start.hpp"
#include "utils/log.hpp"

#include <Windows.h>
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::hud_customizer {

  // =========================================================================
  // 1. Globals (Extern)
  // =========================================================================
  extern bool g_saw_ingame;

  // =========================================================================
  // 2. Main Functions
  // =========================================================================
  auto apply_quick_hides() -> void;
  auto apply_saved_hides() -> void;

  // =========================================================================
  // 3. Helper Functions
  // =========================================================================
  auto resolve_widget(int res_key, bool ingame_map) -> cgwnd*;
  auto hide_widget(int res_key, bool ingame_map) -> void;
  auto show_widget(int res_key, bool ingame_map) -> void;
  auto add_hidden_widget(int res_key, bool ingame_map, const char* label) -> bool;
  auto remove_hidden_widget(int res_key, bool ingame_map) -> bool;
  auto tick_hides() -> void;

  // =========================================================================
  // 4. Named Event Handlers
  // =========================================================================
  auto handle_tick() -> void;
  auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void;

  // =========================================================================
  // 1. Globals Definition
  // =========================================================================
  bool g_saw_ingame = false;

  // =========================================================================
  // 2. Helper Functions Implementation
  // =========================================================================
  auto resolve_widget(int res_key, bool ingame_map) -> cgwnd* {
    if (res_key == 0) return nullptr;
    auto* iface = cg_interface::get();
    if (!iface) return nullptr;
    return iface->get_ui_child(res_key, ingame_map);
  }

  auto hide_widget(int res_key, bool ingame_map) -> void {
    if (cgwnd* wnd = resolve_widget(res_key, ingame_map)) {
      wnd->set_visible(false);
    }
  }

  auto show_widget(int res_key, bool ingame_map) -> void {
    if (cgwnd* wnd = resolve_widget(res_key, ingame_map)) {
      wnd->set_visible(true);
    }
  }

  auto add_hidden_widget(int res_key, bool ingame_map, const char* label) -> bool {
    if (res_key == 0) return false;

    auto& mgr = ext_client::core::config::data().interface_manager;
    for (int i = 0; i < mgr.hidden_count; ++i) {
      if (mgr.hidden[i].res_key == res_key && mgr.hidden[i].ingame_map == ingame_map) {
        return true;
      }
    }

    if (mgr.hidden_count >= ext_client::core::config::core_config::interface_manager_t::max_hidden) {
      return false;
    }

    auto& rule = mgr.hidden[mgr.hidden_count++];
    rule.res_key = res_key;
    rule.ingame_map = ingame_map;
    if (label && label[0] != '\0') {
      std::strncpy(rule.label, label, sizeof(rule.label) - 1);
      rule.label[sizeof(rule.label) - 1] = '\0';
    } else {
      std::snprintf(rule.label, sizeof(rule.label), "0x%X", res_key);
    }

    hide_widget(res_key, ingame_map);
    ext_client::core::config::mark_dirty();
    return true;
  }

  auto remove_hidden_widget(int res_key, bool ingame_map) -> bool {
    auto& mgr = ext_client::core::config::data().interface_manager;
    for (int i = 0; i < mgr.hidden_count; ++i) {
      const auto& rule = mgr.hidden[i];
      if (rule.res_key == res_key && rule.ingame_map == ingame_map) {
        show_widget(res_key, ingame_map);
        for (int j = i + 1; j < mgr.hidden_count; ++j) {
          mgr.hidden[j - 1] = mgr.hidden[j];
        }
        --mgr.hidden_count;
        mgr.hidden[mgr.hidden_count] = {};
        ext_client::core::config::mark_dirty();
        return true;
      }
    }
    return false;
  }

  auto tick_hides() -> void {
    if (g_saw_ingame) {
      if (!cg_interface::get()) {
        g_saw_ingame = false;
        reset_config_promo_hides_cache();
      }
      return;
    }

    static DWORD s_last_check = 0;
    const DWORD now = GetTickCount();
    if (now - s_last_check < 300) {
      return;
    }
    s_last_check = now;

    const bool ready = cg_interface::is_ingame_hud_ready();
    if (ready) {
      g_saw_ingame = true;
      apply_quick_hides();
      if (ext_client::core::config::data().interface_manager.apply_on_startup) {
        apply_saved_hides();
      }
    }
  }

  // =========================================================================
  // 3. Main Functions Implementation
  // =========================================================================
  auto apply_saved_hides() -> void {
    if (!ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
      return;
    }
    const auto& mgr = ext_client::core::config::data().interface_manager;
    for (int i = 0; i < mgr.hidden_count; ++i) {
      const auto& rule = mgr.hidden[i];
      if (rule.res_key != 0) {
        hide_widget(rule.res_key, rule.ingame_map);
      }
    }
  }

  auto apply_quick_hides() -> void {
    if (!ext_client::core::plugin::plugin_manager::get().is_plugin_enabled("hud_customizer")) {
      return;
    }
    auto* iface = cg_interface::get();
    if (!iface) return;

    const auto& cfg = ext_client::core::config::data().interface_hide;

    // Survey button - only hide if configured
    if (cfg.hide_survey) {
      cnif_sro_ingame_start::hide_survey_button(iface);
    }

    apply_config_promo_hides();
  }

  // =========================================================================
  // 4. Named Event Handlers Implementation
  // =========================================================================
  auto handle_tick() -> void {
    tick_hides();
  }

  auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void {
    auto& hides = ext_client::core::config::data().interface_hide;
    const bool old_facebook = hides.hide_facebook;
    const bool old_magic_lamp = hides.hide_magic_lamp;
    const bool old_daily_login = hides.hide_daily_login;
    const bool old_web_item = hides.hide_web_item_alarm;
    const bool old_macro = hides.hide_macro_guide;
    const bool old_survey = hides.hide_survey;

    ui.section("Standard Hides");

    ui.checkbox("Hide Facebook Promo Button", &hides.hide_facebook);
    ui.checkbox("Hide Magic Lamp Button", &hides.hide_magic_lamp);
    ui.checkbox("Hide Daily Login Promo", &hides.hide_daily_login);
    ui.checkbox("Hide Web Item Alarm Status", &hides.hide_web_item_alarm);
    ui.checkbox("Hide Macro Guide Info", &hides.hide_macro_guide);
    ui.checkbox("Hide Survey Button On Bar", &hides.hide_survey);

    if (ui.any_changed()) {
      if (hides.hide_survey != old_survey) {
        if (auto* iface = cg_interface::get()) {
          if (hides.hide_survey) {
            cnif_sro_ingame_start::hide_survey_button(iface);
          } else {
            cnif_sro_ingame_start::show_survey_button(iface);
          }
        }
      }
      if (hides.hide_facebook != old_facebook ||
          hides.hide_magic_lamp != old_magic_lamp ||
          hides.hide_daily_login != old_daily_login ||
          hides.hide_web_item_alarm != old_web_item ||
          hides.hide_macro_guide != old_macro) {
        apply_config_promo_hides();
      }
    }

    ui.spacing();
    ImGui::Separator();
    ui.spacing();

    ImGui::TextColored(ImVec4(0.35f, 0.72f, 0.92f, 1.0f), "Custom Widget Hide Rules");

    static int new_key = 0;
    static bool new_ingame = false;
    static char new_label[64] = "";

    ui.set_next_item_width(120.0f);
    ui.input_int("Widget Res Key (Hex)", &new_key, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
    ui.same_line();
    ImGui::Checkbox("In-Game Map", &new_ingame);
    ui.set_next_item_width(150.0f);
    ui.input_text("Label Name", new_label, sizeof(new_label));
    ui.same_line();
    if (ui.button("Add Hider Rule")) {
      if (new_key != 0) {
        add_hidden_widget(new_key, new_ingame, new_label);
        new_key = 0;
        new_label[0] = '\0';
      }
    }

    ui.spacing();
    auto& im = ext_client::core::config::data().interface_manager;
    if (im.hidden_count > 0) {
      if (ImGui::BeginTable("custom_hides_table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 150.0f))) {
        ImGui::TableSetupColumn("Label");
        ImGui::TableSetupColumn("Key");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (int i = 0; i < im.hidden_count; ++i) {
          const auto& rule = im.hidden[i];
          if (rule.res_key == 0) continue;
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextUnformatted(rule.label);
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("0x%X", rule.res_key);
          ImGui::TableSetColumnIndex(2);
          ImGui::TextUnformatted(rule.ingame_map ? "In-Game" : "Interface");
          ImGui::TableSetColumnIndex(3);
          ImGui::PushID(i);
          if (ImGui::SmallButton("Delete")) {
            remove_hidden_widget(rule.res_key, rule.ingame_map);
          }
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    } else {
      ImGui::TextDisabled("No custom widget hide rules added.");
    }
  }

  auto handle_config_sync() -> void {
    apply_quick_hides();
    if (ext_client::core::config::data().interface_manager.apply_on_startup) {
      apply_saved_hides();
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("hud_customizer", "HUD Customizer");

    ADD_EVENT(EVENT_ON_MENU, handle_menu);
    ADD_EVENT(EVENT_ON_TICK, handle_tick);
    ADD_EVENT(EVENT_ON_CONFIG_SYNC, handle_config_sync);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::hud_customizer
