#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <functional>

namespace ext_client::core::plugin {

  struct plugin_info {
    std::string id;
    std::string display_name;
    std::string description;
    bool enabled = true;
  };

  using plugin_init_fn = std::function<void()>;

  class plugin_manager {
  public:
    static auto get() -> plugin_manager&;

    auto register_plugin(std::string_view id, std::string_view display_name, std::string_view description) -> void;
    auto register_init_fn(plugin_init_fn fn) -> void;
    auto initialize_all() -> void;
    auto is_plugin_enabled(std::string_view id) -> bool;
    auto set_plugin_enabled(std::string_view id, bool enabled) -> void;
    auto get_plugins() -> const std::vector<plugin_info>&;
    auto get_plugins_mutable() -> std::vector<plugin_info>&;

    auto draw_menu_tab(void* raw_ctx) -> void;

  private:
    plugin_manager() = default;
    std::vector<plugin_info> m_plugins;
    std::vector<plugin_init_fn> m_init_fns;
  };

  auto handle_menu_draw(void* raw_ctx) -> void;

  struct plugin_init_registrar {
    plugin_init_registrar(plugin_init_fn fn) {
      plugin_manager::get().register_init_fn(std::move(fn));
    }
  };

  #define REGISTER_PLUGIN(id, display_name, description) \
    ::ext_client::core::plugin::plugin_manager::get().register_plugin(id, display_name, description)

  #ifndef ADD_EVENT_CONCAT2
  #define ADD_EVENT_CONCAT2(a, b) a##b
  #define ADD_EVENT_CONCAT(a, b) ADD_EVENT_CONCAT2(a, b)
  #endif
  #define PLUGIN_INIT(fn) \
    static const ::ext_client::core::plugin::plugin_init_registrar ADD_EVENT_CONCAT(g_init_reg_, __LINE__)(fn)

} // namespace ext_client::core::plugin

