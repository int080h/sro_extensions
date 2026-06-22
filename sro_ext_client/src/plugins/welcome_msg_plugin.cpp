#include "plugins/welcome_msg_plugin.hpp"

#include <Windows.h>
#include <imgui.h>
#include <string>

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "core/core_config.hpp"
#include "utils/imgui_helpers.hpp"
#include "utils/log.hpp"
#include "utils/string.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::welcome_msg {

  auto handle_show_notice(void* raw_ctx) -> void {
    auto* ctx = static_cast<show_notice_context*>(raw_ctx);
    if (!ctx) {
      return;
    }

    const auto& cfg = ext_client::core::config::data().welcome_msg;
    std::wstring wide_cfg_text = ext_client::utils::string::to_wide(cfg.text);

    using ext_client::utils::string::contains_case_insensitive;

    bool is_welcome = contains_case_insensitive(ctx->message, L"Welcome back, we hope you enjoy your stay") ||
                      contains_case_insensitive(ctx->message, L"Welcome back") ||
                      (contains_case_insensitive(ctx->message, L"welcome") && contains_case_insensitive(ctx->message, L"enjoy")) ||
                      (!wide_cfg_text.empty() && contains_case_insensitive(ctx->message, wide_cfg_text));

    if (is_welcome) {
      if (cfg.hide) {
        log_msg("[welcome_msg_plugin] Blocked display of welcome message");
        ctx->is_blocked = true;
        ctx->result = 0;
        return;
      }

      if (cfg.enabled) {
        log_msg("[welcome_msg_plugin] Replacing welcome message with custom text: \"%s\"", cfg.text);
        ctx->is_modified = true;
        ctx->modified_message = wide_cfg_text;
      }
    }
  }

  auto handle_menu(void* raw_ctx) -> void {
    auto* ctx = static_cast<menu_draw_context*>(raw_ctx);
    if (!ctx || !ctx->menu_visible) {
      return;
    }

    if (ImGui::BeginTabItem("Welcome Message")) {
      ext_client::utils::imgui_helpers::section_header("Welcome Message Customization");

      auto& welcome = ext_client::core::config::data().welcome_msg;
      ext_client::utils::imgui_helpers::checkbox_dirty("Enable Welcome Notification Notice", &welcome.enabled);
      ext_client::utils::imgui_helpers::checkbox_dirty("Block Native Notices", &welcome.hide);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 20.0f);
      ext_client::utils::imgui_helpers::input_text_dirty("Custom Message Text", welcome.text, sizeof(welcome.text));
      ImGui::EndTabItem();
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("welcome_msg", "Welcome Message", "Modifies or hides the native in-game welcome notice message.");

    ADD_PLUGIN_EVENT("welcome_msg", event_type::on_show_notice, handle_show_notice);
    ADD_PLUGIN_EVENT("welcome_msg", event_type::on_menu, handle_menu);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::welcome_msg
