#include "core/core_plugin_manager.hpp"
#include "core/core_event_manager.hpp"
#include "core/core_config.hpp"
#include <imgui.h>
#include "utils/log.hpp"
#include <algorithm>
#include <cstdio>

using ext_client::utils::log_msg;

namespace ext_client::core::plugin {

  auto plugin_manager::get() -> plugin_manager& {
    static plugin_manager instance;
    return instance;
  }

  auto plugin_manager::register_plugin(std::string_view id, std::string_view display_name, std::string_view description) -> void {
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(), [&](const plugin_info& p) { return p.id == id; });
    if (it == m_plugins.end()) {
      m_plugins.push_back(plugin_info{std::string(id), std::string(display_name), std::string(description), true});
      log_msg("[plugin_manager] Registered plugin '%s' (%s)", std::string(id).c_str(), std::string(display_name).c_str());
    } else {
      if (it->display_name.empty()) {
        it->display_name = display_name;
        it->description = description;
        log_msg("[plugin_manager] Loaded plugin details for '%s' (%s), state=%s",
                it->id.c_str(), it->display_name.c_str(), it->enabled ? "enabled" : "disabled");
      }
    }
  }

  auto plugin_manager::register_init_fn(plugin_init_fn fn) -> void {
    m_init_fns.push_back(std::move(fn));
  }

  auto plugin_manager::initialize_all() -> void {
    for (auto& fn : m_init_fns) {
      fn();
    }
    m_init_fns.clear();
  }

  auto plugin_manager::is_plugin_enabled(std::string_view id) -> bool {
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(), [&](const plugin_info& p) { return p.id == id; });
    if (it != m_plugins.end()) {
      return it->enabled;
    }
    return false;
  }

  auto plugin_manager::set_plugin_enabled(std::string_view id, bool enabled) -> void {
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(), [&](const plugin_info& p) { return p.id == id; });
    if (it != m_plugins.end()) {
      if (it->enabled != enabled) {
        it->enabled = enabled;
        log_msg("[plugin_manager] Plugin '%s' %s", it->id.c_str(), enabled ? "enabled" : "disabled");
      }
    } else {
      m_plugins.push_back(plugin_info{std::string(id), "", "", enabled});
    }
  }

  auto plugin_manager::get_plugins() -> const std::vector<plugin_info>& {
    return m_plugins;
  }

  auto plugin_manager::get_plugins_mutable() -> std::vector<plugin_info>& {
    return m_plugins;
  }

  auto plugin_manager::draw_menu_tab(void* raw_ctx) -> void {
    auto* ctx = static_cast<ext_client::core::event::menu_draw_context*>(raw_ctx);
    if (!ctx || !ctx->menu_visible) return;

    if (ImGui::BeginTabItem("Plugins")) {
      ImGui::TextColored(ImVec4(0.35f, 0.72f, 0.92f, 1.0f), "Extension Plugins Manager");
      ImGui::Separator();
      ImGui::Spacing();
      ImGui::TextWrapped("Enable or disable client features dynamically. Disabled plugins will have their event listeners blocked and will be hidden from the settings panel.");
      ImGui::Spacing();

      if (ImGui::BeginTable("plugins_list_table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (auto& plugin : m_plugins) {
          if (plugin.display_name.empty()) {
            continue; // Skip dummy entries
          }

          ImGui::TableNextRow();
          
          ImGui::TableSetColumnIndex(0);
          char chk_label[64]{};
          std::sprintf(chk_label, "##chk_%s", plugin.id.c_str());
          bool val = plugin.enabled;
          if (ImGui::Checkbox(chk_label, &val)) {
            set_plugin_enabled(plugin.id, val);
            ext_client::core::config::mark_dirty();
            ext_client::core::config::save();
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%s", plugin.display_name.c_str());

          ImGui::TableSetColumnIndex(2);
          ImGui::TextWrapped("%s", plugin.description.c_str());
        }

        ImGui::EndTable();
      }

      ImGui::EndTabItem();
    }
  }

  auto handle_menu_draw(void* raw_ctx) -> void {
    plugin_manager::get().draw_menu_tab(raw_ctx);
  }

  // Register statically
  ADD_EVENT(ext_client::core::event::event_type::on_menu, handle_menu_draw);

} // namespace ext_client::core::plugin

