#include "menu/menu_shell.hpp"

#include "config/client_config.hpp"
#include "menu/menu_theme.hpp"
#include "menu/menu_ui.hpp"
#include "menu/packet_inspector.hpp"
#include "menu/settings_combat.hpp"
#include "menu/settings_interface.hpp"
#include "menu/settings_panels.hpp"
#include "menu/widget_inspector.hpp"

#include <imgui.h>

#ifndef EXT_CLIENT_DEV_BUILD
#define EXT_CLIENT_DEV_BUILD 0
#endif

namespace ext_client::menu::shell {
  namespace {

    struct popout_windows {
      bool settings = false;
      bool interface_mgr = true;
      bool combat = false;
      bool network = false;
      bool widgets = false;
    };

    popout_windows g_windows{};
    settings_panels::settings_page g_settings_page = settings_panels::settings_page::version_check;

    auto any_popout_open() -> bool {
      return g_windows.settings || g_windows.interface_mgr || g_windows.combat || g_windows.network || g_windows.widgets;
    }

    auto open_settings_page(settings_panels::settings_page page) -> void {
      g_settings_page = page;
      g_windows.settings = true;
    }

    auto show_all_windows() -> void {
      g_windows.settings = true;
      g_windows.interface_mgr = true;
      g_windows.combat = true;
      g_windows.network = true;
#if EXT_CLIENT_DEV_BUILD
      g_windows.widgets = true;
#endif
    }

    auto hide_all_popouts() -> void {
      g_windows.settings = false;
      g_windows.interface_mgr = false;
      g_windows.combat = false;
      g_windows.network = false;
      g_windows.widgets = false;
    }

    auto draw_settings_window() -> void {
      if (!ui::tool_window_begin("Settings##ext_client", &g_windows.settings, ImVec2(720.0f, 560.0f))) {
        return;
      }

      if (ImGui::BeginTable("settings_layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupColumn("subnav", ImGuiTableColumnFlags_WidthFixed, theme::k_settings_subnav_width);
        ImGui::TableSetupColumn("settings_content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        const float subnav_h = ui::avail_height(0.0f, 120.0f);
        if (ImGui::BeginChild("settings_subnav", ImVec2(0.0f, subnav_h), ImGuiChildFlags_Border)) {
          settings_panels::draw_sub_nav(g_settings_page);
          ImGui::EndChild();
        }

        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("settings_content", ImVec2(0.0f, subnav_h), ImGuiChildFlags_Border)) {
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
          settings_panels::draw_page(g_settings_page);
          ImGui::PopStyleVar();
          ImGui::EndChild();
        }

        ImGui::EndTable();
      }

      ui::tool_window_end();
    }

    auto draw_interface_window() -> void {
      if (!ui::tool_window_begin("Interface Manager##ext_client", &g_windows.interface_mgr, ImVec2(920.0f, 640.0f))) {
        return;
      }

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      settings_interface::draw();
      ImGui::PopStyleVar();

      ui::tool_window_end();
    }

    auto draw_combat_window() -> void {
      if (!ui::tool_window_begin("Combat##ext_client", &g_windows.combat, ImVec2(480.0f, 420.0f))) {
        return;
      }

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      settings_combat::draw();
      ImGui::PopStyleVar();

      ui::tool_window_end();
    }

    auto draw_network_window() -> void {
      if (!ui::tool_window_begin("Network##ext_client", &g_windows.network, ImVec2(980.0f, 620.0f))) {
        return;
      }

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      packet_inspector::draw();
      ImGui::PopStyleVar();

      ui::tool_window_end();
    }

    auto draw_widgets_window() -> void {
#if EXT_CLIENT_DEV_BUILD
      if (!ui::tool_window_begin("Widgets##ext_client", &g_windows.widgets, ImVec2(900.0f, 640.0f))) {
        return;
      }

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme::k_content_padding, theme::k_content_padding));
      widget_inspector::draw();
      ImGui::PopStyleVar();

      ui::tool_window_end();
#endif
    }

    auto draw_menu_bar(bool* visible) -> void {
      if (!ImGui::BeginMainMenuBar()) {
        return;
      }

      if (ext_client::config::is_dirty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "*");
        ImGui::SameLine(0.0f, 4.0f);
      }

      if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Version Check")) {
          open_settings_page(settings_panels::settings_page::version_check);
        }
        if (ImGui::MenuItem("Login")) {
          open_settings_page(settings_panels::settings_page::login);
        }
        if (ImGui::MenuItem("Graphics")) {
          open_settings_page(settings_panels::settings_page::graphics);
        }
        if (ImGui::MenuItem("Welcome")) {
          open_settings_page(settings_panels::settings_page::welcome);
        }
        if (ImGui::MenuItem("Advanced")) {
          open_settings_page(settings_panels::settings_page::advanced);
        }
        if (ImGui::MenuItem("App")) {
          open_settings_page(settings_panels::settings_page::app);
        }
        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Interface", nullptr, &g_windows.interface_mgr)) {
      }
      if (ImGui::MenuItem("Combat", nullptr, &g_windows.combat)) {
      }
      if (ImGui::MenuItem("Network", nullptr, &g_windows.network)) {
      }
#if EXT_CLIENT_DEV_BUILD
      if (ImGui::MenuItem("Widgets", nullptr, &g_windows.widgets)) {
      }
#endif

      if (ImGui::BeginMenu("Window")) {
        if (ImGui::MenuItem("Show all")) {
          show_all_windows();
        }
        if (ImGui::MenuItem("Hide all")) {
          hide_all_popouts();
        }
        ImGui::EndMenu();
      }

      const char* config_path = ext_client::config::path();
      const float path_w = ImGui::CalcTextSize(config_path).x;
      const float close_w = ImGui::CalcTextSize("Close").x + ImGui::CalcTextSize("Insert").x + 48.0f;
      const float right_pad = path_w + close_w + 24.0f;
      const float spare = ui::avail_width() - right_pad;
      if (spare > 8.0f) {
        ImGui::SameLine(spare);
      }

      ImGui::TextDisabled("%s", config_path);
      ImGui::SameLine(0.0f, 16.0f);
      if (ImGui::MenuItem("Close", "Insert")) {
        *visible = false;
      }

      ImGui::EndMainMenuBar();
    }

  } // namespace

  auto on_shown() -> void {
    if (!any_popout_open()) {
      g_windows.interface_mgr = true;
    }
  }

  auto draw(bool* visible) -> void {
    if (!visible || !*visible) {
      return;
    }

    draw_menu_bar(visible);
    if (!*visible) {
      return;
    }

    draw_settings_window();
    draw_interface_window();
    draw_combat_window();
    draw_network_window();
    draw_widgets_window();
  }

} // namespace ext_client::menu::shell
