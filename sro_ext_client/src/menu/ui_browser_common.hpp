#pragma once

#include "sdk/cif_manager.hpp"

#include <imgui.h>

#include <cstdint>
#include <unordered_set>
#include <vector>

class cgwnd;

namespace ext_client::menu::ui_browser {

  enum class root_mode : int {
    auto_detect = 0,
    active_process,
    cg_interface,
    ingame_res_map,
    iface_res_map,
  };

  struct ui_root {
    cgwnd* wnd = nullptr;
    const char* label = "none";
    bool res_map_view = false;
  };

  struct root_labels {
    const char* ingame_res_map = "IngameResMap";
    const char* iface_res_map = "CGInterface+0x374";
  };

  auto root_from_widget(cgwnd* wnd) -> ui_root;
  auto resolve_ui_root(root_mode mode, const root_labels& labels = {}) -> ui_root;
  auto enumerate_browse_roots(root_mode mode) -> std::vector<cgwnd*>;

  struct tree_hooks {
    cgwnd* (*selected)(void* ctx) = nullptr;
    void (*on_select)(void* ctx, cgwnd* wnd, int res_key) = nullptr;
    void* ctx = nullptr;
  };

  struct tree_recurse_ctx {
    bool static_only = false;
    bool show_only_visible = false;
    std::unordered_set<cgwnd*>* seen = nullptr;
    tree_hooks hooks{};
  };

  auto recurse_widget_tree(cgwnd* elem, tree_recurse_ctx& ctx) -> void;
  auto draw_res_map_roots_tree(const std::vector<cif_manager::res_map_entry>& entries, tree_recurse_ctx& ctx) -> void;

  auto lookup_info_for(cgwnd* widget,
                       const std::vector<cif_manager::widget_info>& search_results,
                       root_mode mode) -> cif_manager::widget_info;

  auto draw_widget_glow(cgwnd* widget, ImU32 color, float thickness) -> void;

} // namespace ext_client::menu::ui_browser
