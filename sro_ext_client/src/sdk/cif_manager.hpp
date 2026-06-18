#pragma once

#include "cif_static.hpp"

#include <cstdint>
#include <vector>

struct cps_outer_interface;

namespace ext_client::cif_manager {

  enum class widget_kind : std::uint8_t {
    unknown = 0,
    cgwnd,
    cif_wnd,
    cif_static,
    cif_decorated_static,
    cif_facebook_link_alarm,
    cif_magic_lamp_alarm,
    cif_daily_login_alarm,
    cif_button,
    cif_edit,
    calarm_guide_mgr,
    cg_interface,
    cps_title,
    cps_version_check,
    cps_character_select,
  };

  struct widget_info {
    cgwnd* widget = nullptr;
    widget_kind kind = widget_kind::unknown;
    std::uint32_t vftable = 0;
    int control_id = 0;
    int res_map_key = -1;             // key when discovered via res-map walk (-1 = unknown)
    int lookup_res_key = -1;          // resolved key for get_ui_child (parent maps included)
    bool lookup_res_inferred = false; // true when matched via ddj/resinfo, not res-map pointer
    int depth = 0;
    int rect_x = 0;
    int rect_y = 0;
    int rect_w = 0;
    int rect_h = 0;
    bool visible = false;
    char type_name[64]{};
    char res_name[64]{}; // GDR_* from resinfo when catalog is loaded
    char ddj_path[128]{};
    wchar_t text[256]{};
  };

  struct spawn_label_options {
    cgwnd_create_rect rect{};
    std::uint32_t text_color = 0xFFFFFFFF;
    const wchar_t* text = L"";
    bool visible = true;
  };

  auto kind_name(widget_kind kind) -> const char*;
  auto vftable_kind(std::uint32_t vftable) -> widget_kind;
  auto vftable_type_name(std::uint32_t vftable) -> const char*;

  auto is_plain_static(const cgwnd* wnd) -> bool;
  auto is_static(const cgwnd* wnd) -> bool;
  auto as_static_if(cgwnd* wnd) -> cif_static*;
  auto is_same_res_type(const void* res_descriptor, const void* expected) -> bool;

  auto control_id(const cgwnd* wnd) -> int;
  auto unique_id(const cgwnd* wnd) -> int;
  auto is_visible(const cgwnd* wnd) -> bool;
  auto is_live_widget(const cgwnd* wnd) -> bool;
  auto is_cps_outer_process(const void* obj) -> bool;
  auto ui_type_name(const void* obj) -> const char*;
  auto browsable_ui_root(void* obj) -> cgwnd*;
  // Game-native UI root used by sub_D70440 hit-testing (CGFXMainFrame+0x24).
  auto game_ui_root() -> cgwnd*;
  auto topmost_ui_ancestor(cgwnd* wnd) -> cgwnd*;
  auto is_walkable_root(const cgwnd* wnd) -> bool;
  auto resolve_root_from_hover() -> cgwnd*;
  auto read_static_text(const cif_static* label, wchar_t* dst, std::size_t dst_count) -> bool;

  auto find_ui_child(cps_outer_interface* outer, int control_id, bool add_base_key = false) -> cgwnd*;
  auto find_ui_child(cgwnd* outer_as_gwnd, int control_id, bool add_base_key = false) -> cgwnd*;
  auto find_title_child(cps_outer_interface* title, int res_id) -> cgwnd*;
  auto find_title_child(cgwnd* title_as_gwnd, int res_id) -> cgwnd*;

  // dword_1420408 — NIFSroInGame map (sub_4016F0). Not CGInterface+0x374.
  struct ingame_res_lookup {
    int res_key = 0;
    void* raw = nullptr;
    cgwnd* wnd = nullptr;
    bool map_readable = false;
    bool found = false;
    bool live = false;
    std::uint32_t vftable = 0;
  };

  auto find_ingame_res(int res_key) -> cgwnd*;
  auto find_ingame_res_raw(int res_key) -> void*;
  auto diagnose_ingame_res(int res_key) -> ingame_res_lookup;

  struct res_map_entry {
    int key = -1;
    cgwnd* wnd = nullptr;
  };

  auto enumerate_res_map(const void* map_obj, int max_entries = 200) -> std::vector<res_map_entry>;
  auto enumerate_ingame_res_map(int max_entries = 200) -> std::vector<res_map_entry>;
  auto enumerate_iface_res_map(int max_entries = 200) -> std::vector<res_map_entry>;
  auto res_map_key_for(cps_outer_interface* outer, const cgwnd* widget) -> int;
  auto find_res_map_key(const cgwnd* widget) -> int;
  auto read_widget_ddj_path(const cgwnd* wnd, char* dst, std::size_t dst_count) -> bool;
  auto enrich_widget_info(widget_info& info, const cgwnd* walk_root) -> void;
  auto find_title_channel_list_button(cps_outer_interface* title) -> cgwnd*;
  auto find_title_channel_list_button(cgwnd* title_as_gwnd) -> cgwnd*;

  // cginterface_probe_max_id: when root is CGInterface, also call get_ui_child(id) for id in 1..N.
  auto walk(cgwnd* root, int max_depth = 0, int cginterface_probe_max_id = 0) -> std::vector<widget_info>;
  auto walk_static_texts(cgwnd* root, int max_depth = 0, int cginterface_probe_max_id = 0) -> std::vector<widget_info>;

  using walk_visitor_fn = void (*)(cgwnd* wnd, void* ctx);
  using child_visitor_fn = void (*)(cgwnd* child, void* ctx);
  auto walk_each(cgwnd* root, int max_depth, walk_visitor_fn visit, void* ctx) -> void;
  auto walk_ingame_res_roots(walk_visitor_fn visit, void* ctx, int child_depth = 12) -> void;

  // One hierarchy level: intrusive child list + embedded / IFWnd res maps (same edges as walk_each).
  auto for_each_owned_child(cgwnd* parent, child_visitor_fn visit, void* ctx) -> void;
  auto count_owned_children(const cgwnd* parent) -> std::size_t;

  auto client_pointer_pos(int& x, int& y) -> bool;
  // refresh=false: read g_CurrentIfUnderCursor only (no sub_D70440). refresh=true: sync then read.
  auto resolve_hovered_widget(bool refresh = false) -> cgwnd*;

  // max_depth <= 0: walk every node reachable via child_list_ref + map_ref (seen-set stops cycles only).
  inline constexpr int walk_unlimited_depth = 0;

  auto spawn_static_label(cgwnd* parent, const spawn_label_options& options) -> cif_static*;
  auto destroy_widget(cgwnd* widget) -> void;

  auto apply_static_label(cif_static* label, const wchar_t* text, std::uint32_t sro_color) -> void;

} // namespace ext_client::cif_manager
