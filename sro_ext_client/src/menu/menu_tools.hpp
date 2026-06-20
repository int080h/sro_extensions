#pragma once

#include "hooks/net_hook.hpp"
#include "sdk/cgwnd.hpp"
#include "sdk/cif_static.hpp"
#include "menu/packet_decoder.hpp"
#include "menu/packet_inspector.hpp"

#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

// ===========================================================================
// interface_manager
// ===========================================================================

namespace ext_client::menu {

  class interface_manager {
  public:
    static auto get() -> interface_manager&;

    interface_manager(const interface_manager&) = delete;
    interface_manager& operator=(const interface_manager&) = delete;
    interface_manager(interface_manager&&) = delete;
    interface_manager& operator=(interface_manager&&) = delete;

    auto draw() -> void;
    auto draw_quick_hides() -> void;
    auto draw_overlay() -> void;

    struct state;
    auto s() -> state&;

  private:
    interface_manager() = default;
  };

} // namespace ext_client::menu

// ===========================================================================
// widget_inspector
// ===========================================================================

namespace ext_client::menu::widget_inspector {
  auto draw() -> void;
  auto draw_overlay() -> void;
} // namespace ext_client::menu::widget_inspector

// ===========================================================================
// ui_browser
// ===========================================================================

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

  // Menu-specific widget info struct (not a game concept — used only by the debug UI browser).
  struct widget_info {
    cgwnd* widget = nullptr;
    std::uint32_t vftable = 0;
    int control_id = 0;
    int res_map_key = -1;
    int lookup_res_key = -1;
    bool lookup_res_inferred = false;
    int depth = 0;
    int rect_x = 0;
    int rect_y = 0;
    int rect_w = 0;
    int rect_h = 0;
    bool visible = false;
    char type_name[64]{};
    char res_name[64]{};
    char ddj_path[128]{};
    wchar_t text[256]{};
  };

  struct res_map_entry {
    int key = -1;
    cgwnd* wnd = nullptr;
  };

  auto recurse_widget_tree(cgwnd* elem, tree_recurse_ctx& ctx) -> void;
  auto draw_res_map_roots_tree(const std::vector<res_map_entry>& entries, tree_recurse_ctx& ctx) -> void;

  auto lookup_info_for(cgwnd* widget, const std::vector<widget_info>& search_results, root_mode mode) -> widget_info;
  auto draw_widget_glow(cgwnd* widget, ImU32 color, float thickness) -> void;

  // Menu-specific walk utilities (not game concepts — used only by the debug UI browser).
  inline constexpr int walk_unlimited_depth = 0;

  auto walk(cgwnd* root, int max_depth = 0, int cginterface_probe_max_id = 0) -> std::vector<widget_info>;
  auto walk_static_texts(cgwnd* root, int max_depth = 0, int cginterface_probe_max_id = 0) -> std::vector<widget_info>;
  auto enumerate_iface_res_map(int max_entries = 200) -> std::vector<res_map_entry>;
  auto enumerate_ingame_res_map(int max_entries = 200) -> std::vector<res_map_entry>;
  auto enrich_widget_info(widget_info& info, const cgwnd* walk_root) -> void;
  auto res_map_key_for(const cgwnd* widget) -> int;
  auto read_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool;
  auto resolve_hovered_widget(bool refresh) -> cgwnd*;
  auto is_walkable_root(const cgwnd* wnd) -> bool;

  struct spawn_label_options {
    cgwnd_create_rect rect{};
    std::uint32_t text_color = 0xFFFFFFFF;
    const wchar_t* text = L"";
    bool visible = true;
  };

  auto spawn_static_label(cgwnd* parent, const spawn_label_options& options) -> cif_static*;
  auto apply_static_label(cif_static* label, const wchar_t* text, std::uint32_t color) -> void;

} // namespace ext_client::menu::ui_browser
