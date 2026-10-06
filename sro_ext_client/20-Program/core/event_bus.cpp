#include "pch.hpp"
#include "core/event_bus.hpp"
#include "core/plugin_manager.hpp"

namespace ext_client::core::event {
  namespace {
    thread_local const char *g_current_owner = nullptr;
    std::atomic<bool> g_stopping{false};
  } // namespace
  auto dispatch_mutex() -> std::recursive_mutex & {
    static std::recursive_mutex mutex;
    return mutex;
  }
  auto dispatch_stopping() -> bool {
    return g_stopping.load(std::memory_order_acquire);
  }
  auto stop_dispatch() -> void {
    g_stopping.store(true, std::memory_order_release);
    // Wait for callbacks already executing, including nested events.
    std::lock_guard lock(dispatch_mutex());
  }
  auto current_owner() -> const char * {
    return g_current_owner;
  }
  auto resolve_owner_state(const char *owner) -> std::shared_ptr<std::atomic<bool>> {
    return owner ? plugin::plugin_manager::get().owner_state(owner) : nullptr;
  }
  plugin_owner_scope::plugin_owner_scope(const char *owner) : m_previous(g_current_owner) {
    g_current_owner = owner;
  }
  plugin_owner_scope::~plugin_owner_scope() {
    g_current_owner = m_previous;
  }
} // namespace ext_client::core::event
