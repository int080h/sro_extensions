#include "hooks/target_window_hook.hpp"

#include "sdk/cif_target_window.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::target_window {
  namespace {

    make_hook<convention_type::thiscall_t, int, void*, void*> g_populate_target;
    make_hook<convention_type::thiscall_t, void, void*, void*> g_populate_special_mob;
    make_hook<convention_type::thiscall_t, unsigned int, void*> g_update_special_mob;
    hook_group g_hooks;

    auto target_id_active(void* target_id) -> bool {
      return target_id != nullptr && reinterpret_cast<std::uintptr_t>(target_id) != 0;
    }

    auto __fastcall populate_target_detour(void* self, void* /*edx*/, void* target_id) -> int {
      const int result = g_populate_target.call_original(self, target_id);
      if (!target_id_active(target_id)) {
        cif_target_window::clear_active_panel();
      }
      return result;
    }

    auto __fastcall populate_special_mob_detour(void* self, void* /*edx*/, void* target_id) -> void {
      g_populate_special_mob.call_original(self, target_id);
      if (target_id_active(target_id)) {
        cif_target_window::note_active_panel(self);
      } else {
        cif_target_window::clear_active_panel();
      }
    }

    auto __fastcall update_special_mob_detour(void* self) -> unsigned int {
      const unsigned int result = g_update_special_mob.call_original(self);
      if (cif_target_window::is_live_target_panel(self)) {
        cif_target_window::note_active_panel(self);
      }
      return result;
    }

  } // namespace

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets::cif_target_window::functions;
    if (!g_hooks.install(g_populate_target,
                         populate_target,
                         &populate_target_detour,
                         "target_window_hook",
                         "populate_target")) {
      return false;
    }

    if (!g_hooks.install(g_populate_special_mob,
                         populate_special_mob,
                         &populate_special_mob_detour,
                         "target_window_hook",
                         "populate_special_mob")) {
      g_hooks.uninstall();
      return false;
    }

    if (!g_hooks.install(g_update_special_mob,
                         on_update_special_mob,
                         &update_special_mob_detour,
                         "target_window_hook",
                         "update_special_mob")) {
      g_hooks.uninstall();
      return false;
    }

    log_msg("[target_window_hook] Hook installed successfully");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
    cif_target_window::clear_active_panel();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::target_window
