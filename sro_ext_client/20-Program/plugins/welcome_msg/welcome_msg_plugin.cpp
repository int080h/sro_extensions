#include "pch.hpp"
#include "plugins/welcome_msg/welcome_msg_plugin.hpp"

#include <Windows.h>
#include <imgui.h>
#include <string>

#include "core/core_config.hpp"
#include "core/core_event_manager.hpp"
#include "render/menu_builder.hpp"
#include "core/core_plugin_manager.hpp"
#include "utils/log.hpp"
#include "utils/string.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::welcome_msg {

  auto handle_show_notice(show_notice_context& ctx) -> void {
    const auto& cfg = ext_client::core::config::data().welcome_msg;
    std::wstring wide_cfg_text = ext_client::utils::string::to_wide(cfg.text);

    using ext_client::utils::string::contains_case_insensitive;

    bool is_welcome = contains_case_insensitive(ctx.message, L"Welcome back, we hope you enjoy your stay") ||
                      contains_case_insensitive(ctx.message, L"Welcome back") ||
                      (contains_case_insensitive(ctx.message, L"welcome") && contains_case_insensitive(ctx.message, L"enjoy")) ||
                      (!wide_cfg_text.empty() && contains_case_insensitive(ctx.message, wide_cfg_text));

    if (is_welcome) {
      if (cfg.hide) {
        log_msg("[welcome_msg_plugin] Blocked display of welcome message");
        ctx.is_blocked = true;
        ctx.result = 0;
        return;
      }

      if (cfg.enabled) {
        log_msg("[welcome_msg_plugin] Replacing welcome message with custom text: \"%s\"", cfg.text);
        ctx.is_modified = true;
        ctx.modified_message = wide_cfg_text;
      }
    }
  }

  auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void {
    ui.section("Welcome Message Customization");

    auto& welcome = ext_client::core::config::data().welcome_msg;
    ui.checkbox("Enable Welcome Notification Notice", &welcome.enabled);
    ui.checkbox("Block Native Notices", &welcome.hide);
    ui.set_next_item_width(ImGui::GetContentRegionAvail().x - 20.0f);
    ui.input_text("Custom Message Text", welcome.text, sizeof(welcome.text));
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("welcome_msg", "Welcome Message");

    ADD_EVENT(EVENT_ON_MENU, handle_menu);
    ADD_EVENT(EVENT_ON_SHOW_NOTICE, handle_show_notice);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::welcome_msg
