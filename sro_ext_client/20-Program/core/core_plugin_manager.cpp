#include "pch.hpp"
#include "core/core_plugin_manager.hpp"

#include "core/core_event_manager.hpp"
#include "core/core_config.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;

namespace ext_client::core::plugin {

  auto plugin_manager::get() -> plugin_manager & {
    static plugin_manager instance;
    return instance;
  }

  auto plugin_manager::register_plugin(std::string_view id, std::string_view display_name) -> void {
    std::lock_guard lock(m_mutex);
    auto idx_it = m_plugin_index.find(std::string(id));
    if (idx_it == m_plugin_index.end()) {
      const std::size_t index = m_plugins.size();
      m_plugins.push_back(plugin_info{
          std::string(id),
          std::string(display_name),
          true,
      });
      m_states.push_back(std::make_shared<std::atomic<bool>>(m_plugins.back().enabled));
      m_plugin_index.emplace(m_plugins.back().id, index);
      log_msg("[plugin_manager] Registered plugin '%s' (%s)", m_plugins.back().id.c_str(),
              m_plugins.back().display_name.c_str());
    } else {
      auto &info = m_plugins[idx_it->second];
      if (info.display_name.empty()) {
        info.display_name = display_name;
        log_msg("[plugin_manager] Loaded plugin details for '%s' (%s), state=%s", info.id.c_str(),
                info.display_name.c_str(), info.enabled ? "enabled" : "disabled");
      }
    }
  }

  auto plugin_manager::register_init_fn(plugin_init_fn fn) -> void {
    std::lock_guard lock(m_mutex);
    m_init_fns.push_back(std::move(fn));
  }

  auto plugin_manager::initialize_all() -> void {
    std::vector<plugin_init_fn> callbacks;
    {
      std::lock_guard lock(m_mutex);
      callbacks.swap(m_init_fns);
    }
    for (auto &fn : callbacks)
      fn();
  }

  auto plugin_manager::is_plugin_enabled(std::string_view id) -> bool {
    std::lock_guard lock(m_mutex);
    auto it = m_plugin_index.find(std::string(id));
    if (it != m_plugin_index.end()) {
      return m_states[it->second]->load(std::memory_order_acquire);
    }
    return false;
  }

  auto plugin_manager::set_plugin_enabled(std::string_view id, bool enabled) -> void {
    std::lock_guard dispatch_lock(event::dispatch_mutex());
    std::lock_guard lock(m_mutex);
    auto it = m_plugin_index.find(std::string(id));
    if (it != m_plugin_index.end()) {
      auto &info = m_plugins[it->second];
      if (info.enabled != enabled) {
        info.enabled = enabled;
        m_states[it->second]->store(enabled, std::memory_order_release);
        config::mark_dirty();
        log_msg("[plugin_manager] Plugin '%s' %s", info.id.c_str(), enabled ? "enabled" : "disabled");
      }
    } else {
      const std::size_t index = m_plugins.size();
      m_plugins.push_back(plugin_info{std::string(id), "", enabled});
      m_states.push_back(std::make_shared<std::atomic<bool>>(m_plugins.back().enabled));
      m_plugin_index.emplace(m_plugins.back().id, index);
    }
  }

  auto plugin_manager::get_plugins() -> std::vector<plugin_info> {
    std::lock_guard lock(m_mutex);
    auto snapshot = m_plugins;
    for (std::size_t i = 0; i < snapshot.size(); ++i)
      snapshot[i].enabled = m_states[i]->load(std::memory_order_acquire);
    return snapshot;
  }
  auto plugin_manager::owner_state(std::string_view id) -> std::shared_ptr<std::atomic<bool>> {
    std::lock_guard lock(m_mutex);
    auto it = m_plugin_index.find(std::string(id));
    return it == m_plugin_index.end() ? nullptr : m_states[it->second];
  }

} // namespace ext_client::core::plugin
