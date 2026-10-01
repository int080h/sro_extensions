#include "pch.hpp"
#include "plugins/assert/assert_plugin.hpp"

#include <cstring>

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "sdk/game/centity_manager.hpp"
#include "sdk/game/centity_manager_client.hpp"
#include "sdk/game/ccontroler.hpp"
#include "sdk/process/cps_title.hpp"
#include "sdk/render/cg_interface.hpp"
#include "sdk/ui/cgwnd.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::assert_bypass {

#if EXT_CLIENT_DEV_BUILD
  auto run_rtti_smoke_tests() -> void {
    if (auto* mgr = centity_manager::get_singleton(); mgr && centity_manager::is_instance(mgr)) {
      log_msg("[assert_plugin] RTTI smoke: CEntityManager OK");
    }
    if (auto* client = centity_manager_client::get(); client && centity_manager_client::is_instance(client)) {
      log_msg("[assert_plugin] RTTI smoke: CEntityManagerClient OK");
    }
    if (auto* iface = cg_interface::get(); iface && cg_interface::is_instance(iface)) {
      log_msg("[assert_plugin] RTTI smoke: CGInterface OK");
    }
    if (auto* title = ccontroler::active_child_as<cps_title>("CPSTitle"); title && cps_title::is_live(title)) {
      log_msg("[assert_plugin] RTTI smoke: CPSTitle OK");
    }
  }
#endif

  auto handle_assert_report(assert_report_context& ctx) -> void {
    if (ctx.file && std::strstr(ctx.file, "MsgStreamBuffer.h") != nullptr) {
      log_msg("[assert_plugin] MsgStreamBuffer read overflow bypassed: line=%d, file=%s, msg=\"%s\"", ctx.line, ctx.file, ctx.msg ? ctx.msg : "nullptr");
      ctx.handled = true;
      ctx.result = true; // Return true to prevent BSLib's DebugBreak/ExitProcess crash
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("assert_bypass", "Assert Bypass");

    ADD_EVENT(EVENT_ON_ASSERT_REPORT, handle_assert_report);
#if EXT_CLIENT_DEV_BUILD
    run_rtti_smoke_tests();
#endif
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::assert_bypass
