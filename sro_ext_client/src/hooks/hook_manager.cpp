#include "hooks/hook_manager.hpp"

#include "hooks/bslib_hook.hpp"
#include "hooks/cps_character_select_hook.hpp"
#include "render/render_system.hpp"
#include "hooks/cps_quit_hook.hpp"
#include "hooks/cnif_sro_ingame_start_hook.hpp"
#include "menu/interface_manager_runtime.hpp"
#include "menu/debug_profiler.hpp"
#include "hooks/net_hook.hpp"
#include "hooks/quest_hook.hpp"
#include "hooks/cif_target_window_hook.hpp"
#include "hooks/cps_title_hook.hpp"
#include "hooks/cps_version_check_hook.hpp"
#include "hooks/cif_notify_hook.hpp"
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
      {"version_check", cps_version_check_hook::install, cps_version_check_hook::uninstall, nullptr, false, true},
      {"cps_title", cps_title_hook::install, cps_title_hook::uninstall, nullptr, false, true},
      {"net", net::install, net::uninstall, ext_client::hooks::net::register_handlers, false, true},
      {"cnif_sro_ingame_start", cnif_sro_ingame_start_hook::install, cnif_sro_ingame_start_hook::uninstall, nullptr, false, true},
      {"cps_character_select", cps_character_select_hook::install, cps_character_select_hook::uninstall, nullptr, false, true},
      {"cif_notify", cif_notify_hook::install, cif_notify_hook::uninstall, nullptr, false, true},
      {"cif_target_window", cif_target_window_hook::install, cif_target_window_hook::uninstall, nullptr, false, false},
      {"quest", quest::install, quest::uninstall, nullptr, false, true},
      {"bslib", bslib::install, bslib::uninstall, nullptr, false, true},
      {"cps_quit", cps_quit::install, cps_quit::uninstall, nullptr, false, false},
      {"d3d", []() { return ext_client::render::render_system::get().install(); },
             []() { ext_client::render::render_system::get().uninstall(); }, nullptr, true, false},
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
    namespace dp = ext_client::menu::debug_profiler;

    dp::begin_phase(dp::phase_id::sub_cps_title);
    if (dp::is_phase_enabled(dp::phase_id::sub_cps_title)) {
      cps_title_hook::tick();
    }
    dp::end_phase(dp::phase_id::sub_cps_title);

    dp::begin_phase(dp::phase_id::sub_cnif_ingame);
    if (dp::is_phase_enabled(dp::phase_id::sub_cnif_ingame)) {
      cnif_sro_ingame_start_hook::tick();
    }
    dp::end_phase(dp::phase_id::sub_cnif_ingame);

    dp::begin_phase(dp::phase_id::sub_iface_mgr);
    if (dp::is_phase_enabled(dp::phase_id::sub_iface_mgr)) {
      ext_client::menu::interface_manager_runtime::tick();
    }
    dp::end_phase(dp::phase_id::sub_iface_mgr);

    dp::begin_phase(dp::phase_id::sub_cps_version);
    if (dp::is_phase_enabled(dp::phase_id::sub_cps_version)) {
      cps_version_check_hook::tick();
    }
    dp::end_phase(dp::phase_id::sub_cps_version);
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
