#include "pch.hpp"
#include "plugins/net_log/net_log_plugin.hpp"
#include "plugins/net_log/net_log_capture.hpp"
#include "plugins/net_log/net_log_ui.hpp"

#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"

using namespace ext_client::core::event;

namespace ext_client::plugins::net_log {

auto initialize() -> void {
  REGISTER_PLUGIN("net_log", "Network Logs");
  ADD_EVENT(EVENT_ON_MENU, handle_menu);
  ADD_EVENT(EVENT_ON_PACKET, handle_packet);
  ADD_EVENT(EVENT_ON_BACKGROUND_TICK, flush_pending_file);
  ADD_EVENT(EVENT_ON_SHUTDOWN, handle_shutdown);
}

PLUGIN_INIT(initialize);
} // namespace ext_client::plugins::net_log
