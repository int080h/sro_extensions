#include "hooks/hook_manager.hpp"

#include "hooks/assert_hook.hpp"
#include "hooks/character_select_hook.hpp"
#include "hooks/d3d_hook.hpp"
#include "hooks/exit_hooks.hpp"
#include "hooks/intro_hook.hpp"
#include "hooks/interface_hide_hook.hpp"
#include "hooks/interface_manager_runtime.hpp"
#include "hooks/packet_hook.hpp"
#include "hooks/quest_hook.hpp"
#include "hooks/sight_range_hook.hpp"
#include "hooks/target_window_hook.hpp"
#include "hooks/title_hook.hpp"
#include "hooks/version_check_hook.hpp"
#include "hooks/welcome_msg_hook.hpp"
#include "sdk/net_manager.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;

namespace ext_client::hooks::manager {
  namespace {

    using install_fn = bool (*)();
    using uninstall_fn = void (*)();
    using post_install_fn = void (*)();

    struct hook_entry {
      const char* name;
      install_fn install;
      uninstall_fn uninstall;
      post_install_fn post_install;
      bool lazy;
      bool required;
    };

    constexpr hook_entry k_hooks[] = {
      {"version_check", version_check::install, version_check::uninstall, nullptr, false, true},
      {"title", title::install, title::uninstall, nullptr, false, true},
      {"packet", packet::install, packet::uninstall, ext_client::net_manager::register_handlers, false, true},
      {"interface_hide", interface_hide::install, interface_hide::uninstall, nullptr, false, true},
      {"character_select", character_select::install, character_select::uninstall, nullptr, false, true},
      {"welcome_msg", welcome_msg::install, welcome_msg::uninstall, nullptr, false, true},
      {"sight_range", sight_range::install, sight_range::uninstall, nullptr, false, true},
      {"target_window", target_window::install, target_window::uninstall, nullptr, false, false},
      {"intro", intro::install, intro::uninstall, nullptr, false, true},
      {"quest", quest::install, quest::uninstall, nullptr, false, true},
      {"assertion", assertion::install, assertion::uninstall, nullptr, false, true},
      {"exit_hooks", exit_hooks::install, exit_hooks::uninstall, nullptr, false, false},
      {"d3d", d3d::install, d3d::uninstall, nullptr, true, false},
    };

    auto install_entry(const hook_entry& entry) -> bool {
      if (!entry.install()) {
        log_msg("[hook_manager] ERROR: %s failed to install", entry.name);
        return false;
      }
      if (entry.post_install) {
        entry.post_install();
      }
      return true;
    }

    auto uninstall_entry(const hook_entry& entry) -> void {
      entry.uninstall();
    }

  } // namespace

  auto install_all() -> bool {
    bool all_ok = true;
    bool any_installed = false;

    for (const auto& entry : k_hooks) {
      if (entry.lazy) {
        continue;
      }

      if (install_entry(entry)) {
        any_installed = true;
      } else if (entry.required) {
        all_ok = false;
      }
    }

    if (!all_ok && any_installed) {
      log_msg("[hook_manager] rolling back partially installed hooks");
      uninstall_all();
    }

    return all_ok;
  }

  auto uninstall_all() -> void {
    for (auto it = std::end(k_hooks); it != std::begin(k_hooks);) {
      --it;
      uninstall_entry(*it);
    }
  }

  auto tick() -> void {
    title::tick();
    interface_hide::tick();
    interface_manager::tick();
    version_check::tick();
  }

  auto try_install_lazy() -> void {
    for (const auto& entry : k_hooks) {
      if (!entry.lazy) {
        continue;
      }
      entry.install();
    }
  }

} // namespace ext_client::hooks::manager
