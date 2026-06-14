#include "hooks/hook_manager.hpp"

#include "hooks/d3d_hook.hpp"
#include "hooks/packet_hook.hpp"
#include "hooks/interface_hide_hook.hpp"
#include "hooks/title_hook.hpp"
#include "hooks/version_check_hook.hpp"
#include "hooks/character_select_hook.hpp"
#include "hooks/welcome_msg_hook.hpp"
#include "hooks/sight_range_hook.hpp"
#include "hooks/intro_hook.hpp"
#include "hooks/quest_hook.hpp"
#include "hooks/assert_hook.hpp"
#include "sdk/net_manager.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;

namespace ext_client::hooks::manager {

  auto install_all() -> bool {
    bool all_ok = true;

    if (!version_check::install()) {
      log_msg("[hook_manager] ERROR: version_check failed to install");
      all_ok = false;
    }

    if (!title::install()) {
      log_msg("[hook_manager] ERROR: title failed to install");
      all_ok = false;
    }

    if (packet::install()) {
      ext_client::net_manager::register_handlers();
    } else {
      log_msg("[hook_manager] ERROR: packet failed to install");
      all_ok = false;
    }

    if (!interface_hide::install()) {
      log_msg("[hook_manager] ERROR: interface_hide failed to install");
      all_ok = false;
    }

    if (!character_select::install()) {
      log_msg("[hook_manager] ERROR: character_select failed to install");
      all_ok = false;
    }

    if (!welcome_msg::install()) {
      log_msg("[hook_manager] ERROR: welcome_msg failed to install");
      all_ok = false;
    }

    if (!sight_range::install()) {
      log_msg("[hook_manager] ERROR: sight_range failed to install");
      all_ok = false;
    }

    if (!intro::install()) {
      log_msg("[hook_manager] ERROR: intro failed to install");
      all_ok = false;
    }

    if (!quest::install()) {
      log_msg("[hook_manager] ERROR: quest failed to install");
      all_ok = false;
    }

    if (!assertion::install()) {
      log_msg("[hook_manager] ERROR: assertion failed to install");
      all_ok = false;
    }

    return all_ok;
  }

  auto uninstall_all() -> void {
    assertion::uninstall();
    quest::uninstall();
    intro::uninstall();
    sight_range::uninstall();
    character_select::uninstall();
    welcome_msg::uninstall();
    interface_hide::uninstall();
    packet::uninstall();
    title::uninstall();
    version_check::uninstall();
    d3d::uninstall();
  }

  auto tick() -> void {
    title::tick();
    interface_hide::tick();
    version_check::tick();
  }


} // namespace ext_client::hooks::manager
