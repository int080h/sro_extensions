#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"

namespace ext_client::core::event {

  auto event_manager::get() -> event_manager& {
    static event_manager instance;
    return instance;
  }

  auto event_manager::register_event(event_type type, event_callback callback) -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners[type].push_back(listener_entry{"", std::move(callback)});
  }

  auto event_manager::register_plugin_event(event_type type, std::string_view plugin_id, event_callback callback) -> void {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners[type].push_back(listener_entry{std::string(plugin_id), std::move(callback)});
  }

  auto event_manager::dispatch(event_type type, void* context) -> void {
    std::vector<listener_entry> targets;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto it = m_listeners.find(type);
      if (it != m_listeners.end()) {
        targets = it->second;
      }
    }

    for (const auto& entry : targets) {
      if (entry.callback) {
        if (entry.plugin_id.empty() || plugin::plugin_manager::get().is_plugin_enabled(entry.plugin_id)) {
          entry.callback(context);
        }
      }
    }
  }

} // namespace ext_client::core::event
