#include "pch.hpp"
#include "render/plugin_menu.hpp"
#include "render/menu_builder.hpp"
#include "core/plugin_manager.hpp"
#include <imgui.h>
using ext_client::render::menu::menu_builder;
namespace ext_client::render {
  namespace event = core::event;
  auto draw_plugins_tab_content(menu_builder &ui) -> void {
    const auto& plugins = core::plugin::plugin_manager::get().get_plugins();
    std::size_t active_count = 0;
    for (const auto& p : plugins) {
      if (p.enabled) ++active_count;
    }

    ui.section("Extension Plugins Manager");
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Active Modules: %zu / %zu", active_count, plugins.size());
    ImGui::TextDisabled("Toggle modular plugins on or off in real time. Changes persist across game sessions.");
    ui.spacing();

    if (ImGui::BeginTable("plugins_list_table", 2,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("Plugin Module", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableHeadersRow();

      for (auto &plugin : core::plugin::plugin_manager::get().get_plugins()) {
        if (plugin.display_name.empty()) {
          continue;
        }

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(plugin.id.c_str());
        bool enabled = plugin.enabled;
        if (ui.checkbox("##enabled", &enabled)) {
          core::plugin::plugin_manager::get().set_plugin_enabled(plugin.id, enabled);
        }
        ImGui::PopID();

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", plugin.display_name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", plugin.id.c_str());
      }

      ImGui::EndTable();
    }
  }

  auto draw_plugin_tabs(event::menu_draw_context *ctx) -> void {
    if (!ctx || !ctx->menu_visible) {
      return;
    }

    // -------------------------------------------------------------------------
    // 1. In-Game HUD & Visuals
    // -------------------------------------------------------------------------
    if (ImGui::BeginTabItem("In-Game HUD & Visuals")) {
      if (ImGui::BeginTabBar("HudVisualsTabs")) {
        if (ImGui::BeginTabItem("Target HUD")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("target_hp", ui);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("3D World ESP & Radar")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("hud_esp", ui);
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
      ImGui::EndTabItem();
    }

    // -------------------------------------------------------------------------
    // 2. Interface & Tweaks
    // -------------------------------------------------------------------------
    if (ImGui::BeginTabItem("Interface & Tweaks")) {
      if (ImGui::BeginTabBar("InterfaceTweaksTabs")) {
        if (ImGui::BeginTabItem("HUD Customizer")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("hud_customizer", ui);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Welcome Message")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("welcome_msg", ui);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Login Screen")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("title", ui);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Loading Screens")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("version_check", ui);
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
      ImGui::EndTabItem();
    }

    // -------------------------------------------------------------------------
    // 3. Plugins Manager
    // -------------------------------------------------------------------------
    if (ImGui::BeginTabItem("Plugins Manager")) {
      menu_builder ui;
      draw_plugins_tab_content(ui);
      ImGui::EndTabItem();
    }

    // -------------------------------------------------------------------------
    // 4. Developer Suite
    // -------------------------------------------------------------------------
    if (ImGui::BeginTabItem("Developer Suite")) {
      if (ImGui::BeginTabBar("DeveloperSuiteTabs")) {
        if (ImGui::BeginTabItem("Network Packet Logs")) {
          menu_builder ui;
          ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner("net_log", ui);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Engine Diagnostics & Guards")) {
          menu_builder ui;
          ui.section("Active Engine Guards & Crash Protections");
          ImGui::BulletText("Assert Bypass: MsgStreamBuffer overflow protection active (anti-crash)");
          ImGui::BulletText("Quest Fallback: Missing quest ID dummy object substitution active");
          ImGui::BulletText("Version Check: Version verification handshake & login splash active");
          ImGui::BulletText("D3D9 Hook: Safe EndScene / Device Reset recovery active");
          ui.spacing();
          ui.section("Developer Visual Diagnostics");
          ImGui::BulletText("To toggle 3D Skeleton bones, wireframe OBB mesh boxes, or screen snaplines,");
          ImGui::BulletText("navigate to 'In-Game HUD & Visuals' -> '3D World ESP & Radar' -> 'Developer Diagnostics'.");
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
      ImGui::EndTabItem();
    }

    // -------------------------------------------------------------------------
    // 5. Dynamic Extension Tabs (Forward Compatibility for Future Plugins)
    // -------------------------------------------------------------------------
    for (const auto &plugin : core::plugin::plugin_manager::get().get_plugins()) {
      if (!plugin.enabled || plugin.display_name.empty()) {
        continue;
      }
      if (plugin.id == "target_hp" || plugin.id == "hud_esp" || plugin.id == "hud_customizer" ||
          plugin.id == "welcome_msg" || plugin.id == "title" || plugin.id == "version_check" ||
          plugin.id == "net_log" || plugin.id == "assert_bypass" || plugin.id == "character_select" ||
          plugin.id == "quest") {
        continue;
      }
      if (!::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().has_owner(plugin.id.c_str())) {
        continue;
      }

      if (ImGui::BeginTabItem(plugin.display_name.c_str())) {
        menu_builder ui;
        ::ext_client::core::event::event_handler<EVENT_ON_MENU>::instance().trigger_owner(plugin.id.c_str(), ui);
        ImGui::EndTabItem();
      }
    }
  }
} // namespace ext_client::render
