#pragma once

#include "core/event_bus.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ext_client::render::menu {
  class menu_builder;
}

namespace ext_client::core::plugin {

  struct plugin_info {
    std::string id;
    std::string display_name;
    bool enabled = true;
  };

  using plugin_init_fn = std::function<void()>;

  class plugin_manager {
  public:
    static auto get() -> plugin_manager &;

    auto register_plugin(std::string_view id, std::string_view display_name) -> void;
    auto register_init_fn(plugin_init_fn fn) -> void;
    auto initialize_all() -> void;
    auto is_plugin_enabled(std::string_view id) -> bool;
    auto set_plugin_enabled(std::string_view id, bool enabled) -> void;
    auto get_plugins() -> std::vector<plugin_info>;
    auto owner_state(std::string_view id) -> std::shared_ptr<std::atomic<bool>>;

  private:
    plugin_manager() = default;
    std::mutex m_mutex;
    std::vector<plugin_info> m_plugins;
    std::vector<std::shared_ptr<std::atomic<bool>>> m_states;
    std::unordered_map<std::string, std::size_t> m_plugin_index;
    std::vector<plugin_init_fn> m_init_fns;
  };

  struct plugin_init_registrar {
    plugin_init_registrar(plugin_init_fn fn) { plugin_manager::get().register_init_fn(std::move(fn)); }
  };

#define REGISTER_PLUGIN(id, name)                                                                                      \
  ::ext_client::core::plugin::plugin_manager::get().register_plugin(id, name);                                         \
  ::ext_client::core::event::plugin_owner_scope ADD_EVENT_CONCAT(_plugin_owner_scope_, __LINE__)(id)

#ifndef ADD_EVENT_CONCAT2
#define ADD_EVENT_CONCAT2(a, b) a##b
#define ADD_EVENT_CONCAT(a, b) ADD_EVENT_CONCAT2(a, b)
#endif
#define PLUGIN_INIT(fn)                                                                                                \
  static const ::ext_client::core::plugin::plugin_init_registrar ADD_EVENT_CONCAT(g_init_reg_, __LINE__)(fn)
} // namespace ext_client::core::plugin
