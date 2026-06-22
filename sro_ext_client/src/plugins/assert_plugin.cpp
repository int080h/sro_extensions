#include "plugins/assert_plugin.hpp"

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::assert_bypass {

  auto handle_assert_report(void* raw_ctx) -> void {
    auto* ctx = static_cast<assert_report_context*>(raw_ctx);
    if (ctx && ctx->file && std::strstr(ctx->file, "MsgStreamBuffer.h") != nullptr) {
      log_msg("[assert_plugin] MsgStreamBuffer read overflow bypassed: line=%d, file=%s, msg=\"%s\"",
              ctx->line, ctx->file, ctx->msg ? ctx->msg : "nullptr");
      ctx->handled = true;
      ctx->result = true; // Return true to prevent BSLib's DebugBreak/ExitProcess crash
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("assert_bypass", "Assert Bypass", "Bypasses engine assert error reports to prevent game crashes.");

    ADD_PLUGIN_EVENT("assert_bypass", event_type::on_assert_report, handle_assert_report);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::assert_bypass
