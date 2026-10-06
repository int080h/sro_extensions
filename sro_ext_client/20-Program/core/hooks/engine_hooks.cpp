#include "pch.hpp"
#include "core/hooks/engine_hooks.hpp"
#include "core/event_bus.hpp"
#include "utils/hooks.hpp"

namespace ext_client::core::hooks::engine_hooks {
  namespace {
    std::atomic<bool> g_installed{false};
  }
  auto install_all() -> bool {
    std::lock_guard lock(utils::hook_lifecycle_mutex());
    if (g_installed.load())
      return true;
    if (!client_hooks::install() || !network_hooks::install() || !shutdown_hooks::install()) {
      uninstall_all();
      return false;
    }
    g_installed.store(true);
    return true;
  }
  auto uninstall_all() -> bool {
    std::lock_guard lock(utils::hook_lifecycle_mutex());
    bool ok = d3d_hooks::uninstall();
    ok = shutdown_hooks::uninstall() && ok;
    ok = network_hooks::uninstall() && ok;
    ok = client_hooks::uninstall() && ok;
    if (ok)
      g_installed.store(false);
    return ok;
  }
  auto is_installed() -> bool {
    return g_installed.load();
  }
  auto is_render_installed() -> bool {
    return d3d_hooks::is_installed();
  }
  auto install_lazy() -> void {
    d3d_hooks::install_lazy();
  }
  auto tick() -> void {
    static DWORD last_tick = 0;
    const auto now = GetTickCount();
    if (now == last_tick)
      return;
    last_tick = now;
    TRIGGER_EVENT(EVENT_ON_TICK);
  }
} // namespace ext_client::core::hooks::engine_hooks
