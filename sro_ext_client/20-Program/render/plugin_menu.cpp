#include "pch.hpp"
#include "render/plugin_menu.hpp"
#include "render/menu_builder.hpp"
#include "core/core_plugin_manager.hpp"
#include <imgui.h>
using ext_client::render::menu::menu_builder;
namespace ext_client::render {
  namespace event = core::event;
  auto draw_plugins_tab_content(menu_builder &ui) -> void {
    ui.section("Extension Plugins Manager");
    ui.spacing();

    if (ImGui::BeginTable("plugins_list_table", 2,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
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
      }

      ImGui::EndTable();
    }
  }

  auto draw_plugin_tabs(event::menu_draw_context *ctx) -> void {
    if (!ctx || !ctx->menu_visible) {
      return;
    }

    if (ImGui::BeginTabItem("Plugins")) {
      menu_builder ui;
      draw_plugins_tab_content(ui);
      ImGui::EndTabItem();
    }

    for (const auto &plugin : core::plugin::plugin_manager::get().get_plugins()) {
      if (!plugin.enabled || plugin.display_name.empty()) {
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
