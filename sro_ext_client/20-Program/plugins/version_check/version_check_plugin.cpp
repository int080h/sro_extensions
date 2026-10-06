#include "pch.hpp"
#include "plugins/version_check/version_check_plugin.hpp"
#include "plugins/version_check/version_check_runtime.hpp"
#include "plugins/version_check/version_check_menu.hpp"

#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"

using namespace ext_client::core::event;

namespace ext_client::plugins::version_check {

auto initialize() -> void {
  REGISTER_PLUGIN("version_check", "Loading Screens");

  ADD_EVENT(EVENT_ON_MENU, handle_menu);
  ADD_EVENT(EVENT_ON_VERSION_CHECK_CREATE, handle_version_check_create);
  ADD_EVENT(EVENT_ON_VERSION_CHECK_UPDATE, handle_version_check_update);
  ADD_EVENT(EVENT_ON_SET_CHILD_PROCESS, handle_set_child_process);
  ADD_EVENT(EVENT_ON_LOAD_INTRO_CAMERA, handle_load_intro_camera);
  ADD_EVENT(EVENT_ON_SHUTDOWN, handle_shutdown);
  ADD_EVENT(EVENT_ON_TICK, handle_tick);
}

PLUGIN_INIT(initialize);
} // namespace ext_client::plugins::version_check
