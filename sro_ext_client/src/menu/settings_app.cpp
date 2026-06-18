#include "ext_client.hpp"

#include "menu/settings_app.hpp"

#include "menu/menu_ui.hpp"

#include <imgui.h>

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
