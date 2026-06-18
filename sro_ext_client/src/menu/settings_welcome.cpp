#include "ext_client.hpp"

#include "menu/settings_welcome.hpp"

#include "menu/menu_ui.hpp"

#include <imgui.h>

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
