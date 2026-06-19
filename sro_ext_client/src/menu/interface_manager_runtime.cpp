#include "menu/interface_manager_runtime.hpp"

#include "utils/client_config.hpp"
#include "sdk/cg_interface.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_manager.hpp"
#include "utils/hooks.hpp"

#include <cstring>

namespace ext_client::menu::interface_manager_runtime {
  namespace {

    constexpr std::uint32_t k_enforce_every_n_ticks = 30;
    std::uint32_t g_enforce_counter = 0;

    auto resolve_widget(int res_key, bool ingame_map) -> cgwnd* {
      if (res_key == 0) {
        return nullptr;
      }
      if (ingame_map) {
        return cif_manager::find_ingame_res(res_key);
      }
      auto* iface = cg_interface::get();
      if (!iface) {
        return nullptr;
      }
      if (cgwnd* root = cif_manager::browsable_ui_root(iface)) {
        return cif_manager::find_ui_child(root, res_key, true);
      }
      return nullptr;
    }

    auto hide_widget(int res_key, bool ingame_map) -> void {
      if (cgwnd* wnd = resolve_widget(res_key, ingame_map)) {
        cgwnd::set_visible(wnd, false);
      }
    }

    auto show_widget(int res_key, bool ingame_map) -> void {
      if (cgwnd* wnd = resolve_widget(res_key, ingame_map)) {
        cgwnd::set_visible(wnd, true);
      }
    }

  } // namespace

  auto apply_saved_hides() -> void {
    const auto& mgr = ext_client::config::data().interface_manager;
    for (int i = 0; i < mgr.hidden_count; ++i) {
      const auto& rule = mgr.hidden[i];
      if (rule.res_key != 0) {
        hide_widget(rule.res_key, rule.ingame_map);
      }
    }
  }

  auto tick() -> void {
    const auto& mgr = ext_client::config::data().interface_manager;
    if (mgr.hidden_count == 0) {
      return;
    }
    if (!cg_interface::is_ingame_hud_ready()) {
      return;
    }

    if (!ext_client::utils::every_n_ticks(g_enforce_counter, k_enforce_every_n_ticks)) {
      return;
    }

    apply_saved_hides();
  }

  auto add_hidden_widget(int res_key, bool ingame_map, const char* label) -> bool {
    if (res_key == 0) {
      return false;
    }

    auto& mgr = ext_client::config::data().interface_manager;
    if (is_hidden_widget(res_key, ingame_map)) {
      return true;
    }

    if (mgr.hidden_count >= ext_client::config::settings::interface_manager_t::max_hidden) {
      return false;
    }

    auto& rule = mgr.hidden[mgr.hidden_count++];
    rule.res_key = res_key;
    rule.ingame_map = ingame_map;
    if (label && label[0] != '\0') {
      std::strncpy(rule.label, label, sizeof(rule.label) - 1);
      rule.label[sizeof(rule.label) - 1] = '\0';
    } else {
      std::snprintf(rule.label, sizeof(rule.label), "0x%X", res_key);
    }

    hide_widget(res_key, ingame_map);
    ext_client::config::mark_dirty();
    if (ext_client::config::data().general.save_on_change) {
      ext_client::config::save();
    }
    return true;
  }

  auto remove_hidden_widget(int res_key, bool ingame_map) -> bool {
    auto& mgr = ext_client::config::data().interface_manager;
    for (int i = 0; i < mgr.hidden_count; ++i) {
      const auto& rule = mgr.hidden[i];
      if (rule.res_key == res_key && rule.ingame_map == ingame_map) {
        show_widget(res_key, ingame_map);
        for (int j = i + 1; j < mgr.hidden_count; ++j) {
          mgr.hidden[j - 1] = mgr.hidden[j];
        }
        --mgr.hidden_count;
        mgr.hidden[mgr.hidden_count] = {};
        ext_client::config::mark_dirty();
        if (ext_client::config::data().general.save_on_change) {
          ext_client::config::save();
        }
        return true;
      }
    }
    return false;
  }

  auto is_hidden_widget(int res_key, bool ingame_map) -> bool {
    const auto& mgr = ext_client::config::data().interface_manager;
    for (int i = 0; i < mgr.hidden_count; ++i) {
      const auto& rule = mgr.hidden[i];
      if (rule.res_key == res_key && rule.ingame_map == ingame_map) {
        return true;
      }
    }
    return false;
  }

} // namespace ext_client::menu::interface_manager_runtime
