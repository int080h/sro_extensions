#include "pch.hpp"
#include "plugins/title/title_plugin.hpp"
#include "plugins/title/title_runtime.hpp"

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"

using namespace ext_client::core::event;

namespace ext_client::plugins::title {

auto initialize() -> void {
  REGISTER_PLUGIN("title", "Login Screen");

  ADD_EVENT(EVENT_ON_MENU, handle_menu);
  ADD_EVENT(EVENT_ON_TICK, handle_tick);
}

PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::title
