#pragma once

#include <Windows.h>
#include <imgui.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <vector>
#include <string>

#include "core/core_event_manager.hpp"
#include "core/core_config.hpp"
#include "utils/msvc9_stl.hpp"
#include "sdk/cps_title.hpp"
#include "sdk/cif_static.hpp"

namespace ext_client::plugins::title {

  // =========================================================================
  // 1. Layout Parsing Struct
  // =========================================================================
  class title_login_layout {
  public:
    struct widget_rect {
      int res_id = -1;
      int x = 0;
      int y = 0;
      int width = 0;
      int height = 0;
      bool valid = false;
    };

    widget_rect login_frame{};
    widget_rect id_label{};
    widget_rect id_edit{};
    widget_rect password_label{};
    widget_rect password_edit{};
    widget_rect server_label{};
    widget_rect server_value{};
    widget_rect server_button{};
    widget_rect channel_label{};
    widget_rect channel_value{};
    widget_rect channel_button{};
    widget_rect channel_name{};

    struct title_res_id {
      static constexpr int channel_message = 15;
      static constexpr int id_edit = 41;
      static constexpr int password_edit = 42;
      static constexpr int server_value = 43;
      static constexpr int server_button = 44;
      static constexpr int channel_value = 45;
      static constexpr int channel_button = 46;
      static constexpr int id_label = 101;
      static constexpr int password_label = 102;
      static constexpr int server_label = 103;
      static constexpr int channel_label = 104;
    };

    auto title_ui_child(cps_title* title, int res_id) const -> cgwnd*;
    auto translated_rect(widget_rect rect, int x_adjust, int y_adjust) const -> widget_rect;
    auto fallback_rect_for_id(int res_id, widget_rect& out) const -> bool;
    auto apply_widget_rect(cps_title* title, int target_res_id, const widget_rect& rect, int base_x, int base_y) const -> bool;
    auto hide_title_child(cps_title* title, int res_id) const -> void;
    auto load_named_or_id(const char* gdr_name, int fallback_res_id, widget_rect& out) -> bool;
    auto load_from_game(cps_title* title) -> bool;
    auto apply_eu_frame(cps_title* title, cgwnd* frame) const -> bool;
    auto restore_eu_frame(cps_title* title, cgwnd* frame) const -> bool;
  };

  // =========================================================================
  // 2. Enums and Constants
  // =========================================================================
  enum class label_type {
    none,
    data,
    exe,
  };

  inline constexpr wchar_t k_default_data_fmt[] = L"Client Data Version %d.%03d";
  inline constexpr wchar_t k_default_exe_fmt[] = L"Client Exe Version %.3f";
  inline constexpr char k_login_frame_id_ddj[] = "login_window_2016_id.ddj";
  inline constexpr char k_login_frame_email_ddj[] = "login_window_2016_email.ddj";
  inline constexpr char k_login_frame_default_ddj[] = "login_window.ddj";
  inline constexpr char k_login_frame_eu_located_ddj[] = "login_window_eu_located.ddj";

  inline constexpr int k_big_logo_default_y = 448;
  inline constexpr int k_logo_default_y = 403;
  inline constexpr int k_version_label_max_w = 520;
  inline constexpr int k_version_label_max_h = 40;

  // =========================================================================
  // 3. Structures
  // =========================================================================
  struct logo_baseline_entry {
    cgwnd* widget = nullptr;
    int y = 0;
  };

  struct label_discover_ctx {
    cps_title* title = nullptr;
    bool need_data = true;
    bool need_exe = true;
  };

  struct login_frame_walk_ctx {
    cgwnd* found = nullptr;
  };

  struct channel_list_find_ctx {
    cgwnd* channel_combo = nullptr;
    cgwnd* found = nullptr;
  };

  struct title_list_button_collect_ctx {
    cgwnd* buttons[8]{};
    int count = 0;
  };

  struct logo_walk_ctx {
    const char* ddj_name = nullptr;
    cgwnd* found = nullptr;
  };

  // =========================================================================
  // 4. Globals (Extern)
  // =========================================================================
  extern cif_static* g_data_version_label;
  extern cif_static* g_exe_version_label;
  extern double g_original_exe_version_value;
  extern cps_title* g_bound_title;
  extern bool g_saw_title;

  extern void* g_last_hid_channel_list_btn;
  extern cgwnd* g_cached_channel_list_btn;
  extern cps_title* g_cached_channel_list_title;

  extern cgwnd* g_cached_big_logo;
  extern cgwnd* g_cached_logo;

  extern title_login_layout g_login_layout;
  extern std::vector<logo_baseline_entry> g_logo_baselines;

  // =========================================================================
  // 5. Main Functions & Helpers
  // =========================================================================
  auto captured_version_label_count() -> int;
  auto apply_version_labels() -> void;
  auto apply_version_label_layout() -> void;
  auto apply_from_control() -> void;

  auto control_panel() -> ext_client::core::config::core_config::title_t&;
  auto resolve_wide_fmt(const char* utf8_fmt, wchar_t* dst, std::size_t dst_count, const wchar_t* fallback) -> const wchar_t*;

  auto walk_logo_search_cb(cgwnd* child, void* ctx) -> int;
  auto walk_channel_list_find_cb(cgwnd* child, void* ctx) -> int;
  auto walk_login_frame_cb(cgwnd* child, void* ctx) -> int;
  auto walk_title_list_btn_cb(cgwnd* child, void* ctx) -> int;

  auto discover_version_labels(cps_title* title) -> void;
  auto hide_version_labels() -> void;
  auto reset_logo_baselines() -> void;
  auto is_title_screen_active(const cps_title* title) -> bool;
  auto apply_version_label_overrides() -> void;
  auto restore_original_version_labels() -> void;
  auto apply_version_label_layout_internal() -> void;
  auto apply_logo_layout_internal(cps_title* title) -> void;
  auto apply_logo_layout(cps_title* title) -> void;
  auto apply_title_ui(cps_title* title, bool force_apply) -> void;
  auto bind_title_instance(cps_title* title) -> void;
  auto tick() -> void;

  // =========================================================================
  // 6. Named Event Handlers
  // =========================================================================
  auto handle_tick(void* raw_ctx) -> void;
  auto handle_menu(void* raw_ctx) -> void;

} // namespace ext_client::plugins::title
