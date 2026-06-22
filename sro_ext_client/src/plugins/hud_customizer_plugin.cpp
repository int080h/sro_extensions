#include "plugins/hud_customizer_plugin.hpp"
#include "core/core_plugin_manager.hpp"
#include "core/core_config.hpp"
#include "utils/imgui_helpers.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::hud_customizer {

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
    return reinterpret_cast<cgwnd*>(iface->get_ui_child(res_key, ingame_map));
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
    const bool ready = cg_interface::is_ingame_hud_ready();
    if (ready) {
      if (!g_saw_ingame) {
        g_saw_ingame = true;
        apply_quick_hides();
        if (ext_client::core::config::data().interface_manager.apply_on_startup) {
          apply_saved_hides();
        }
      }
    } else {
      g_saw_ingame = false;
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

    // Survey button
    if (cfg.hide_survey) {
      cnif_sro_ingame_start::hide_survey_button(iface);
    } else {
      cnif_sro_ingame_start::show_survey_button(iface);
    }

    // Alarm strip icons (Facebook, Magic Lamp, etc.)
    auto* mgr = calram_guide_mgr_wnd::current();
    if (mgr) {
      unsigned int mask = 0;
      using target = calram_guide_mgr_wnd::promo_target;
      if (cfg.hide_facebook) mask |= static_cast<unsigned>(target::facebook);
      if (cfg.hide_magic_lamp) mask |= static_cast<unsigned>(target::magic_lamp);
      if (cfg.hide_daily_login) mask |= static_cast<unsigned>(target::daily_login);
      if (cfg.hide_web_item_alarm) mask |= static_cast<unsigned>(target::web_item_alarm);
      if (cfg.hide_macro_guide) mask |= static_cast<unsigned>(target::macro_guide);

      if (mask != 0) {
        mgr->apply_promo_hide(static_cast<target>(mask));
      } else {
        mgr->apply_promo_show(target::none);
      }
    }
  }

  // =========================================================================
  // 4. Named Event Handlers Implementation
  // =========================================================================
  auto handle_tick(void*) -> void {
    tick_hides();
  }

  auto handle_menu(void* raw_ctx) -> void {
    auto* ctx = static_cast<menu_draw_context*>(raw_ctx);
    if (!ctx) return;

    if (ctx->menu_visible) {
      if (ImGui::BeginTabItem("HUD Customizer")) {
        auto& hides = ext_client::core::config::data().interface_hide;
        ext_client::utils::imgui_helpers::section_header("Standard Hides");

        bool changed = false;
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Facebook Promo Button", &hides.hide_facebook);
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Magic Lamp Button", &hides.hide_magic_lamp);
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Daily Login Promo", &hides.hide_daily_login);
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Web Item Alarm Status", &hides.hide_web_item_alarm);
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Macro Guide Info", &hides.hide_macro_guide);
        changed |= ext_client::utils::imgui_helpers::checkbox_dirty("Hide Survey Button On Bar", &hides.hide_survey);

        if (changed) {
          apply_quick_hides();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.35f, 0.72f, 0.92f, 1.0f), "Custom Widget Hide Rules");
        
        static int new_key = 0;
        static bool new_ingame = false;
        static char new_label[64] = "";
        
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("Widget Res Key (Hex)", &new_key, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::SameLine();
        ImGui::Checkbox("In-Game Map", &new_ingame);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("Label Name", new_label, sizeof(new_label));
        ImGui::SameLine();
        if (ImGui::Button("Add Hider Rule")) {
          if (new_key != 0) {
            add_hidden_widget(new_key, new_ingame, new_label);
            new_key = 0;
            new_label[0] = '\0';
          }
        }

        ImGui::Spacing();
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

        ImGui::EndTabItem();
      }
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("hud_customizer", "HUD Customizer", "Allows custom hides, scaling, and manipulation of various HUD widgets.");

    ADD_PLUGIN_EVENT("hud_customizer", event_type::on_tick, handle_tick);
    ADD_PLUGIN_EVENT("hud_customizer", event_type::on_menu, handle_menu);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::hud_customizer
